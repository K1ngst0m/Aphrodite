#include "engine.h"

#include "common/common.h"
#include "common/logger.h"
#include "common/profiler.h"

#include "api/capture.h"
#include "api/vulkan/device.h"
#include "ui/ui.h"

namespace aph
{
[[maybe_unused]] static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    ::vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, ::vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const ::vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (!pCallbackData->pMessage)
    {
        return VK_TRUE;
    }
    static std::mutex errMutex; // Mutex for thread safety
    static uint32_t errCount = 0;

    // Skip general messages if loader logs are disabled
    auto* debugData = static_cast<Engine::DebugCallbackData*>(pUserData);
    if (messageType == ::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral && !debugData->enableDeviceInitLogs)
    {
        return VK_FALSE;
    }

    std::stringstream msg;
    if (messageType != ::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
    {
        uint32_t frameId = debugData->frameId;
        msg << "[frame:" << frameId << "] ";
    }
    else
    {
        msg << "[general] ";
    }

    for (uint32_t idx = 0; idx < pCallbackData->objectCount; idx++)
    {
        auto& obj = pCallbackData->pObjects[idx];
        if (obj.pObjectName)
        {
            msg << "[name: " << obj.pObjectName << "]";
        }
    }

    msg << " >>> ";

    msg << std::string{pCallbackData->pMessage};

    switch (messageSeverity)
    {
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        VK_LOG_DEBUG("%s", msg.str());
        break;
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        VK_LOG_INFO("%s", msg.str());
        break;
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        VK_LOG_WARN("%s", msg.str());
        break;
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
    {
        std::lock_guard<std::mutex> lock(errMutex);
        if (++errCount > 10)
        {
            VK_LOG_ERR("Too many errors, exit.");
            DebugBreak();
        }
        VK_LOG_ERR("%s", msg.str().c_str());
        DebugBreak();
    }
    break;

    default:
        break;
    }
    return VK_FALSE;
}

Engine::Engine(const EngineConfig& config)
    : m_config(config)
{
    APH_PROFILER_SCOPE();

    // Setup debug callback data
    m_debugCallbackData.frameId = m_frameIdx;
    m_debugCallbackData.enableDeviceInitLogs = config.getEnableDeviceInitLogs();

    // Setup variables needed for engine initialization
    WindowSystemCreateInfo windowSystemInfo{};
    vk::InstanceCreateInfo instanceCreateInfo{};
    vk::DeviceCreateInfo deviceCreateInfo{};
    vk::SwapChainCreateInfo swapChainCreateInfo{};
    ResourceLoaderCreateInfo resourceLoaderCreateInfo{};
    UICreateInfo uiCreateInfo{};
    uint32_t gpuIdx = 0;

    // Initialize timer
    m_timer.set(TIMER_TAG_GLOBAL);

    //
    // 1. Create window system
    //
    {
        windowSystemInfo = config.getWindowSystemCreateInfo();
        windowSystemInfo.width = config.getWidth();
        windowSystemInfo.height = config.getHeight();

        m_pWindowSystem = WindowSystem::Create(windowSystemInfo);
    }

    //
    // 2. Create Vulkan instance
    //
    {
        APH_PROFILER_SCOPE();
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        instanceCreateInfo = config.getInstanceCreateInfo();

        // Configure window system extensions
        auto windowExtensions = m_pWindowSystem->getRequiredExtensions();
        for (const auto& ext : windowExtensions)
        {
            instanceCreateInfo.explicitExtensions.push_back(ext);
        }

#ifdef APH_DEBUG
        // Configure debug and validation features
        instanceCreateInfo.features.enableSurface = true;
        instanceCreateInfo.features.enableSurfaceCapabilities = true;
        instanceCreateInfo.features.enablePhysicalDeviceProperties2 = true;
        instanceCreateInfo.features.enableValidation = true;
        instanceCreateInfo.features.enableDebugUtils = true;

        // Configure capture support
        instanceCreateInfo.features.enableCapture = config.getDeviceCreateInfo().enabledFeatures.capture;

        // Setup debug callback
        instanceCreateInfo.debugCreateInfo.setPUserData(&m_debugCallbackData);
        instanceCreateInfo.debugCreateInfo.setPfnUserCallback(&debugCallback);
#endif

        // Create the instance
        APH_VERIFY_RESULT(vk::Instance::Create(instanceCreateInfo, &m_pInstance));
    }

    //
    // 3. Create logical device
    //
    {
        deviceCreateInfo = config.getDeviceCreateInfo();
        deviceCreateInfo.pPhysicalDevice = m_pInstance->getPhysicalDevices(gpuIdx);
        deviceCreateInfo.pInstance = m_pInstance;

        m_pDevice = vk::Device::Create(deviceCreateInfo);
        VK_LOG_INFO("Select Device [%d].", gpuIdx);
        APH_ASSERT(m_pDevice != nullptr);
    }

    //
    // 4. Create post-device resources in parallel
    //
    {
        auto postDeviceGroup = m_taskManager.createTaskGroup("post device object creation");

        // Configure and create swapchain
        swapChainCreateInfo = config.getSwapChainCreateInfo();
        swapChainCreateInfo.pInstance = m_pInstance;
        swapChainCreateInfo.pWindowSystem = m_pWindowSystem.get();
        swapChainCreateInfo.pQueue = m_pDevice->getQueue(QueueType::Graphics);

        // Task 1: Create swapchain
        postDeviceGroup->addTask(
            [](const vk::SwapChainCreateInfo& createInfo, vk::SwapChain** ppSwapchain, vk::Device* pDevice) -> TaskType
            {
                auto result = pDevice->create(createInfo);
                if (result.success())
                {
                    *ppSwapchain = result.value();
                }
                co_return result;
            }(swapChainCreateInfo, &m_pSwapChain, m_pDevice.get()));

        // Task 2: Create render graphs
        m_frameGraph.resize(config.getMaxFrames());
        postDeviceGroup->addTask(
            [](SmallVector<std::unique_ptr<RenderGraph>>& graphs, vk::Device* pDevice) -> TaskType
            {
                for (auto& graph : graphs)
                {
                    graph = std::make_unique<RenderGraph>(pDevice);
                    if (!graph)
                    {
                        co_return {Result::RuntimeError, "Failed to initialized render graph."};
                    }
                }
                co_return Result::Success;
            }(m_frameGraph, m_pDevice.get()));

        // Task 3: Create resource loader
        resourceLoaderCreateInfo = config.getResourceLoaderCreateInfo();
        postDeviceGroup->addTask(
            [](std::unique_ptr<ResourceLoader>& resourceLoader, vk::Device* pDevice,
               const ResourceLoaderCreateInfo& createInfo) -> TaskType
            {
                ResourceLoaderCreateInfo loaderCreateInfo = createInfo;
                loaderCreateInfo.pDevice = pDevice;

                resourceLoader = std::make_unique<ResourceLoader>(loaderCreateInfo);
                if (!resourceLoader)
                {
                    co_return {Result::RuntimeError, "Failed to initialized resource loader."};
                }
                co_return Result::Success;
            }(m_pResourceLoader, m_pDevice.get(), resourceLoaderCreateInfo));

        // Submit first batch of tasks
        APH_VERIFY_RESULT(postDeviceGroup->submit());

        //
        // 5. Initialize user interface
        //
        uiCreateInfo = config.getUICreateInfo();
        uiCreateInfo.pInstance = m_pInstance;
        uiCreateInfo.pDevice = m_pDevice.get();
        uiCreateInfo.pSwapchain = m_pSwapChain;
        uiCreateInfo.pWindow = m_pWindowSystem.get();

        postDeviceGroup->addTask(
            [](const UICreateInfo& createInfo, std::unique_ptr<UI>& ui) -> TaskType
            {
                ui = aph::createUI(createInfo);
                if (!ui)
                {
                    co_return {Result::RuntimeError, "Failed to initialized UI."};
                }
                co_return Result::Success;
            }(uiCreateInfo, m_ui));

        APH_VERIFY_RESULT(postDeviceGroup->submit());
    }

    //
    // 6. Initialize debugging and capture tools (if enabled)
    //
    {
        if (m_config.getEnableCapture())
        {
            m_pDeviceCapture = std::make_unique<DeviceCapture>();
            if (auto res = m_pDeviceCapture->initialize(); res.success())
            {
                VK_LOG_INFO("Renderdoc plugin loaded.");
            }
            else
            {
                VK_LOG_WARN("Failed to load renderdoc plugin: %s", res.toString());
            }
        }
    }
}

Engine::~Engine()
{
    APH_PROFILER_SCOPE();
    for (auto& graph : m_frameGraph)
    {
        graph.reset();
    }

    m_pResourceLoader->cleanup();
    m_ui->shutdown();
    m_pDevice->destroy(m_pSwapChain);
    vk::Device::Destroy(m_pDevice.get());
    vk::Instance::Destroy(m_pInstance);
};

void Engine::update()
{
    APH_PROFILER_SCOPE();
    // {
    //     m_pUI->update();
    // }
}

void Engine::unload()
{
    APH_PROFILER_SCOPE();
    // {
    //     m_pUI->unload();
    // }
};

void Engine::load()
{
    APH_PROFILER_SCOPE();
    // {
    //     m_pUI->load();
    // }
};

void Engine::render()
{
    APH_PROFILER_SCOPE();
    m_timer.set(TIMER_TAG_FRAME);
    // m_pDevice->begineCapture();
    m_frameGraph[m_frameIdx]->execute();
    // m_pDevice->endCapture();
    m_frameCPUTime = m_timer.interval(TIMER_TAG_FRAME);
}

coro::generator<Engine::FrameResource> Engine::loop()
{
    while (m_pWindowSystem->update())
    {
        update();
        m_frameIdx = (m_frameIdx + 1) % m_config.getMaxFrames();
        m_debugCallbackData.frameId = m_frameIdx;
        co_yield FrameResource{
            .pGraph = m_frameGraph[m_frameIdx].get(),
            .frameIdx = m_frameIdx,
        };
        render();
    }
}

coro::generator<RenderGraph*> Engine::setupGraph()
{
    APH_PROFILER_SCOPE();
    for (auto& pGraph : m_frameGraph)
    {
        co_yield pGraph.get();
    }
}
} // namespace aph
