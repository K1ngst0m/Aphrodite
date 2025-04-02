#include "engine.h"

#include "common/common.h"
#include "common/logger.h"
#include "common/profiler.h"

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

    std::stringstream msg;
    if (messageType != ::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
    {
        uint32_t frameId = *(uint32_t*)pUserData;
        msg << "[fr:" << frameId << "] ";
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

    msg << std::string{ pCallbackData->pMessage };

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
            throw aph::TracedException();
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

    m_timer.set(TIMER_TAG_GLOBAL);

    // create window system
    {
        WindowSystemCreateInfo wsi_create_info = config.getWindowSystemCreateInfo();

        wsi_create_info.width = config.getWidth();
        wsi_create_info.height = config.getHeight();

        m_pWindowSystem = WindowSystem::Create(wsi_create_info);
    }

    // create instance
    {
        APH_PROFILER_SCOPE();
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        auto instanceCreateInfo = config.getInstanceCreateInfo();

        auto requiredExtensions = m_pWindowSystem->getRequiredExtensions();

#ifdef APH_DEBUG
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        instanceCreateInfo.enabledLayers.push_back("VK_LAYER_KHRONOS_validation");

        {
            APH_PROFILER_SCOPE();
            ::vk::DebugUtilsMessengerCreateInfoEXT& debug_create_info = instanceCreateInfo.debugCreateInfo;
            debug_create_info
                .setMessageSeverity(::vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                    ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                    ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
                .setMessageType(::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                ::vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                ::vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                ::vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
                .setPUserData(&m_frameIdx)
                .setPfnUserCallback(&debugCallback);
        }
#endif
        instanceCreateInfo.enabledExtensions = std::move(requiredExtensions);
        APH_VR(vk::Instance::Create(instanceCreateInfo, &m_pInstance));
    }

    // create device
    {
        uint32_t gpuIdx = 0;
        auto createInfo = config.getDeviceCreateInfo();

        createInfo.pPhysicalDevice = m_pInstance->getPhysicalDevices(gpuIdx);
        createInfo.pInstance = m_pInstance;

        m_pDevice = vk::Device::Create(createInfo);
        VK_LOG_INFO("Select Device [%d].", gpuIdx);
        APH_ASSERT(m_pDevice != nullptr);
    }

    auto postDeviceGroup = m_taskManager.createTaskGroup("post device object creation");

    {
        auto createInfo = config.getSwapChainCreateInfo();

        createInfo.pInstance = m_pInstance;
        createInfo.pWindowSystem = m_pWindowSystem.get();
        createInfo.pQueue = m_pDevice->getQueue(QueueType::Graphics);

        postDeviceGroup->addTask(
            [](const vk::SwapChainCreateInfo& createInfo, vk::SwapChain** ppSwapchain, vk::Device* pDevice) -> TaskType
            {
                auto result = pDevice->create(createInfo, ppSwapchain);
                co_return result;
            }(createInfo, &m_pSwapChain, m_pDevice.get()));

        m_frameGraph.resize(config.getMaxFrames());

        postDeviceGroup->addTask(
            [](SmallVector<std::unique_ptr<RenderGraph>>& graphs, vk::Device* pDevice) -> TaskType
            {
                for (auto& graph : graphs)
                {
                    graph = std::make_unique<RenderGraph>(pDevice);
                    if (!graph)
                    {
                        co_return { Result::RuntimeError, "Failed to initialized render graph." };
                    }
                }
                co_return Result::Success;
            }(m_frameGraph, m_pDevice.get()));

        postDeviceGroup->addTask(
            [](std::unique_ptr<ResourceLoader>& resourceLoader, vk::Device* pDevice,
               const ResourceLoaderCreateInfo& createInfo) -> TaskType
            {
                ResourceLoaderCreateInfo loaderCreateInfo = createInfo;
                loaderCreateInfo.pDevice = pDevice;

                resourceLoader = std::make_unique<ResourceLoader>(loaderCreateInfo);
                if (!resourceLoader)
                {
                    co_return { Result::RuntimeError, "Failed to initialized resource loader." };
                }
                co_return Result::Success;
            }(m_pResourceLoader, m_pDevice.get(), config.getResourceLoaderCreateInfo()));
        APH_VR(postDeviceGroup->submit());
    }

    {
        auto uiCreateInfo = config.getUICreateInfo();

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
                    co_return { Result::RuntimeError, "Failed to initialized resource loader." };
                }
                co_return Result::Success;
            }(uiCreateInfo, m_ui));

        APH_VR(postDeviceGroup->submit());
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
