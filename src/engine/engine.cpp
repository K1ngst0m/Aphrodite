#include "engine.h"
#include "debug.h"

#include "common/common.h"
#include "common/logger.h"
#include "common/profiler.h"

#include "api/capture.h"
#include "api/vulkan/device.h"
#include "ui/ui.h"

namespace aph
{

// Static factory function to create engine with custom config
Expected<Engine*> Engine::Create(const EngineConfig& config)
{
    APH_PROFILER_SCOPE();

    // Create the engine with minimal initialization in constructor
    auto* pEngine = new Engine(config);
    if (!pEngine)
    {
        return {Result::RuntimeError, "Failed to allocate Engine instance"};
    }

    // Complete the initialization process
    Result initResult = pEngine->initialize(config);
    if (!initResult.success())
    {
        delete pEngine;
        return initResult;
    }

    return pEngine;
}

// Static destroy function to properly clean up the engine
void Engine::Destroy(Engine* pEngine)
{
    if (pEngine)
    {
        APH_PROFILER_SCOPE();

        // Clean up render graphs
        for (auto* graph : pEngine->m_frameGraph)
        {
            RenderGraph::Destroy(graph);
        }
        pEngine->m_frameGraph.clear();

        // Clean up resources in proper order
        if (pEngine->m_pResourceLoader)
        {
            ResourceLoader::Destroy(pEngine->m_pResourceLoader);
        }

        if (pEngine->m_pWindowSystem)
        {
            WindowSystem::Destroy(pEngine->m_pWindowSystem);
        }

        if (pEngine->m_ui)
        {
            UI::Destroy(pEngine->m_ui);
        }

        if (pEngine->m_pDevice && pEngine->m_pSwapChain)
        {
            pEngine->m_pDevice->destroy(pEngine->m_pSwapChain);
        }

        if (pEngine->m_pDevice)
        {
            vk::Device::Destroy(pEngine->m_pDevice);
        }

        if (pEngine->m_pInstance)
        {
            vk::Instance::Destroy(pEngine->m_pInstance);
        }

        if (pEngine->m_pDeviceCapture)
        {
            DeviceCapture::Destroy(pEngine->m_pDeviceCapture);
        }

        // Finally delete the engine instance
        delete pEngine;
    }
}

Engine::Engine(const EngineConfig& config)
    : m_config(config)
{
    // Initialize timer
    m_timer.set(TIMER_TAG_GLOBAL);

    // Setup minimal debug callback data
    m_debugCallbackData.frameId = m_frameIdx;
    m_debugCallbackData.enableDeviceInitLogs = config.getEnableDeviceInitLogs();
}

Result Engine::initialize(const EngineConfig& config)
{
    APH_PROFILER_SCOPE();

    // Setup variables needed for engine initialization
    WindowSystemCreateInfo windowSystemInfo{};
    vk::InstanceCreateInfo instanceCreateInfo{};
    vk::DeviceCreateInfo deviceCreateInfo{};
    vk::SwapChainCreateInfo swapChainCreateInfo{};
    ResourceLoaderCreateInfo resourceLoaderCreateInfo{};
    UICreateInfo uiCreateInfo{};
    uint32_t gpuIdx = 0;

    //
    // 1. Create window system
    //
    {
        windowSystemInfo = config.getWindowSystemCreateInfo();
        windowSystemInfo.width = config.getWidth();
        windowSystemInfo.height = config.getHeight();

        auto windowSystemResult = WindowSystem::Create(windowSystemInfo);
        APH_RETURN_IF_ERROR(windowSystemResult);
        m_pWindowSystem = windowSystemResult.value();
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
        auto instanceResult = vk::Instance::Create(instanceCreateInfo);
        APH_RETURN_IF_ERROR(instanceResult);
        m_pInstance = instanceResult.value();
    }

    //
    // 3. Create logical device
    //
    {
        deviceCreateInfo = config.getDeviceCreateInfo();
        deviceCreateInfo.pPhysicalDevice = m_pInstance->getPhysicalDevices(gpuIdx);
        deviceCreateInfo.pInstance = m_pInstance;

        auto deviceResult = vk::Device::Create(deviceCreateInfo);
        APH_RETURN_IF_ERROR(deviceResult);
        m_pDevice = deviceResult.value();

        VK_LOG_INFO("Select Device [%d].", gpuIdx);
    }

    //
    // 4. Create post-device resources in parallel
    //
    {
        auto postDeviceGroup = m_taskManager.createTaskGroup("post device object creation");

        // Configure and create swapchain
        swapChainCreateInfo = config.getSwapChainCreateInfo();
        swapChainCreateInfo.pInstance = m_pInstance;
        swapChainCreateInfo.pWindowSystem = m_pWindowSystem;
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
            }(swapChainCreateInfo, &m_pSwapChain, m_pDevice));

        // Task 2: Create render graphs
        m_frameGraph.resize(config.getMaxFrames());
        postDeviceGroup->addTask(
            [](SmallVector<RenderGraph*>& graphs, vk::Device* pDevice) -> TaskType
            {
                for (size_t i = 0; i < graphs.size(); i++)
                {
                    auto graphResult = RenderGraph::Create(pDevice);
                    if (!graphResult.success())
                    {
                        // Clean up previously created graphs
                        for (size_t j = 0; j < i; j++)
                        {
                            RenderGraph::Destroy(graphs[j]);
                        }
                        co_return {graphResult};
                    }
                    graphs[i] = graphResult.value();
                }
                co_return Result::Success;
            }(m_frameGraph, m_pDevice));

        // Task 3: Create resource loader
        resourceLoaderCreateInfo = config.getResourceLoaderCreateInfo();
        postDeviceGroup->addTask(
            [](const ResourceLoaderCreateInfo& createInfo, ResourceLoader** ppResourceLoader,
               vk::Device* pDevice) -> TaskType
            {
                ResourceLoaderCreateInfo loaderCreateInfo = createInfo;
                loaderCreateInfo.pDevice = pDevice;

                auto loaderResult = ResourceLoader::Create(loaderCreateInfo);
                if (!loaderResult.success())
                {
                    co_return {loaderResult.error().code, loaderResult.error().message};
                }
                *ppResourceLoader = loaderResult.value();
                co_return Result::Success;
            }(resourceLoaderCreateInfo, &m_pResourceLoader, m_pDevice));

        // Submit first batch of tasks
        APH_RETURN_IF_ERROR(postDeviceGroup->submit());

        //
        // 5. Initialize user interface
        //
        uiCreateInfo = config.getUICreateInfo();
        uiCreateInfo.pInstance = m_pInstance;
        uiCreateInfo.pDevice = m_pDevice;
        uiCreateInfo.pSwapchain = m_pSwapChain;
        uiCreateInfo.pWindow = m_pWindowSystem;

        postDeviceGroup->addTask(
            [](const UICreateInfo& createInfo, UI** ppUI) -> TaskType
            {
                auto uiResult = UI::Create(createInfo);
                if (!uiResult.success())
                {
                    co_return {uiResult.error().code, uiResult.error().message};
                }
                *ppUI = uiResult.value();
                co_return Result::Success;
            }(uiCreateInfo, &m_ui));

        APH_RETURN_IF_ERROR(postDeviceGroup->submit());
    }

    //
    // 6. Initialize debugging and capture tools (if enabled)
    //
    {
        if (m_config.getEnableCapture())
        {
            auto captureResult = DeviceCapture::Create();
            if (!captureResult.success())
            {
                VK_LOG_WARN("Failed to load renderdoc plugin: %s", captureResult.error().message);
                // Non-fatal error, just log warning
            }
            else
            {
                m_pDeviceCapture = captureResult.value();
                VK_LOG_INFO("Renderdoc plugin loaded.");
            }
        }
    }

    return Result::Success;
}

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
            .pGraph = m_frameGraph[m_frameIdx],
            .frameIdx = m_frameIdx,
        };
        render();
    }
}

coro::generator<RenderGraph*> Engine::setupGraph()
{
    APH_PROFILER_SCOPE();
    for (auto* pGraph : m_frameGraph)
    {
        co_yield pGraph;
    }
}
} // namespace aph
