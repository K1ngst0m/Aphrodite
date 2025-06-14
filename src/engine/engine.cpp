#include "engine.h"
#include "debug.h"

#include "common/logger.h"
#include "common/profiler.h"

#include "api/capture.h"
#include "api/vulkan/device.h"
#include "ui/ui.h"

namespace aph
{

// Static factory function to create engine with custom config
auto Engine::Create(const EngineConfig& config) -> Expected<Engine*>
{
    APH_PROFILER_SCOPE();

    // Create the engine with minimal initialization in constructor
    auto* pEngine = new Engine(config);
    if (!pEngine)
    {
        return { Result::RuntimeError, "Failed to allocate Engine instance" };
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
    APH_ASSERT(pEngine);

    {
        APH_PROFILER_SCOPE();

        APH_ASSERT(pEngine->m_pFrameComposer);
        FrameComposer::Destroy(pEngine->m_pFrameComposer);

        // Clean up resources in proper order
        APH_ASSERT(pEngine->m_pResourceLoader);
        ResourceLoader::Destroy(pEngine->m_pResourceLoader);

        APH_ASSERT(pEngine->m_pWindowSystem);
        WindowSystem::Destroy(pEngine->m_pWindowSystem);

        APH_ASSERT(pEngine->m_ui);
        UI::Destroy(pEngine->m_ui);

        // Clean up material registry
        APH_ASSERT(pEngine->m_pMaterialRegistry);
        MaterialRegistry::Destroy(pEngine->m_pMaterialRegistry);
        pEngine->m_pMaterialRegistry = nullptr;

        APH_ASSERT(pEngine->m_pSwapChain);
        pEngine->m_pDevice->destroy(pEngine->m_pSwapChain);

        APH_ASSERT(pEngine->m_pDevice);
        vk::Device::Destroy(pEngine->m_pDevice);

        APH_ASSERT(pEngine->m_pInstance);
        vk::Instance::Destroy(pEngine->m_pInstance);

        if (pEngine->m_pDeviceCapture != nullptr)
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
    m_timer.set(TimerTag::eTimerTagGlobal);

    m_debugCallbackData.frameId              = 0;
    m_debugCallbackData.enableDeviceInitLogs = config.getEnableDeviceInitLogs();
}

auto Engine::initialize(const EngineConfig& config) -> Result
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
        windowSystemInfo               = config.getWindowSystemCreateInfo();
        windowSystemInfo.width         = config.getWidth();
        windowSystemInfo.height        = config.getHeight();
        windowSystemInfo.enableHighDPI = config.isHighDPIEnabled();

        if (config.isHighDPIEnabled())
        {
            CM_LOG_INFO("High DPI scaling enabled");
        }

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
        instanceCreateInfo.features.enableWindowSystem              = true;
        instanceCreateInfo.features.enablePhysicalDeviceProperties2 = true;
        instanceCreateInfo.features.enableValidation                = true;
        instanceCreateInfo.features.enableDebugUtils                = true;

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
        deviceCreateInfo                 = config.getDeviceCreateInfo();
        deviceCreateInfo.pPhysicalDevice = m_pInstance->getPhysicalDevices(gpuIdx);
        deviceCreateInfo.pInstance       = m_pInstance;
        deviceCreateInfo.enableDebug     = config.getEnableDeviceDebug();

        auto deviceResult = vk::Device::Create(deviceCreateInfo);
        APH_RETURN_IF_ERROR(deviceResult);
        m_pDevice = deviceResult.value();

        VK_LOG_INFO("Select Device [%d].", gpuIdx);
    }

    //
    // 4. Create material registry (before other resources that might depend on it)
    //
    {
        auto materialRegistryResult = MaterialRegistry::Create();
        if (!materialRegistryResult.success())
        {
            return materialRegistryResult;
        }
        m_pMaterialRegistry = materialRegistryResult.value();

        CM_LOG_INFO("Material registry initialized");
    }

    //
    // 5. Create post-device resources in parallel
    //
    {
        auto* postDeviceGroup = m_taskManager.createTaskGroup("post device object creation");

        // Configure and create swapchain
        swapChainCreateInfo               = config.getSwapChainCreateInfo();
        swapChainCreateInfo.pInstance     = m_pInstance;
        swapChainCreateInfo.pWindowSystem = m_pWindowSystem;
        swapChainCreateInfo.pQueue        = m_pDevice->getQueue(QueueType::Graphics);

        // Create swapchain
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

        // Create resource loader
        resourceLoaderCreateInfo                   = config.getResourceLoaderCreateInfo();
        resourceLoaderCreateInfo.pMaterialRegistry = m_pMaterialRegistry;
        postDeviceGroup->addTask(
            [](const ResourceLoaderCreateInfo& createInfo, ResourceLoader** ppResourceLoader,
               vk::Device* pDevice) -> TaskType
            {
                ResourceLoaderCreateInfo loaderCreateInfo = createInfo;
                loaderCreateInfo.pDevice                  = pDevice;

                // Ensure forceUncached setting is passed from config
                if (loaderCreateInfo.forceUncached)
                {
                    CM_LOG_INFO("Engine using forceUncached=true for resource loading");
                }
                else
                {
                    CM_LOG_INFO("Engine using forceUncached=false (caching enabled)");
                }

                auto loaderResult = ResourceLoader::Create(loaderCreateInfo);
                if (!loaderResult.success())
                {
                    co_return { loaderResult.error().code, loaderResult.error().message };
                }
                *ppResourceLoader = loaderResult.value();
                co_return Result::Success;
            }(resourceLoaderCreateInfo, &m_pResourceLoader, m_pDevice));

        // Submit first batch of tasks
        APH_RETURN_IF_ERROR(postDeviceGroup->submit());

        //
        // 5.1 Initialize user interface
        //
        uiCreateInfo                    = config.getUICreateInfo();
        uiCreateInfo.pInstance          = m_pInstance;
        uiCreateInfo.pDevice            = m_pDevice;
        uiCreateInfo.pSwapchain         = m_pSwapChain;
        uiCreateInfo.pWindow            = m_pWindowSystem;
        uiCreateInfo.breadcrumbsEnabled = config.getEnableUIBreadcrumbs();

        postDeviceGroup->addTask(
            [](const UICreateInfo& createInfo, UI** ppUI) -> TaskType
            {
                auto uiResult = UI::Create(createInfo);
                if (!uiResult.success())
                {
                    co_return { uiResult.error().code, uiResult.error().message };
                }
                *ppUI = uiResult.value();
                co_return Result::Success;
            }(uiCreateInfo, &m_ui));

        //
        // 5.2 Create frame composer
        //
        FrameComposerCreateInfo frameComposerCreateInfo{ .pDevice         = m_pDevice,
                                                         .pResourceLoader = m_pResourceLoader,
                                                         .frameCount      = config.getMaxFrames() };

        postDeviceGroup->addTask(
            [](const FrameComposerCreateInfo& createInfo, FrameComposer** ppComposer) -> TaskType
            {
                auto result = FrameComposer::Create(createInfo);
                if (!result.success())
                {
                    co_return { result.error().code, result.error().message };
                }
                *ppComposer = result.value();
                co_return Result::Success;
            }(frameComposerCreateInfo, &m_pFrameComposer));

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

    m_timer.set(TimerTag::eTimerTagFrame);
    return Result::Success;
}

void Engine::update()
{
    APH_PROFILER_SCOPE();
    m_frameCPUTime = m_timer.interval(TimerTag::eTimerTagFrame);
    m_timer.set(TimerTag::eTimerTagFrame);
}

void Engine::render()
{
    APH_PROFILER_SCOPE();
    // m_pDevice->begineCapture();
    m_pFrameComposer->getCurrentGraph()->build(m_pSwapChain);
    m_pFrameComposer->getCurrentGraph()->execute();
    // m_pDevice->endCapture();
}

auto Engine::loop() -> coro::generator<FrameComposer::FrameResource>
{
    while (m_pWindowSystem->update())
    {
        update();

        auto frameResource          = m_pFrameComposer->nextFrame();
        m_debugCallbackData.frameId = frameResource.frameIndex;
        co_yield frameResource;

        render();
    }
}

auto Engine::getInstance() const -> vk::Instance*
{
    return m_pInstance;
}

auto Engine::getSwapchain() const -> vk::SwapChain*
{
    return m_pSwapChain;
}

auto Engine::getUI() const -> UI*
{
    return m_ui;
}

auto Engine::getFrameComposer() const -> FrameComposer*
{
    return m_pFrameComposer;
}

auto Engine::getResourceLoader() const -> ResourceLoader*
{
    return m_pResourceLoader;
}

auto Engine::getDevice() const -> vk::Device*
{
    return m_pDevice;
}

auto Engine::getWindowSystem() const -> WindowSystem*
{
    return m_pWindowSystem;
}

auto Engine::getConfig() const -> const EngineConfig&
{
    return m_config;
}

auto Engine::getResourceForceUncached() const -> bool
{
    return m_config.getResourceForceUncached();
}

auto Engine::getElapsedTime() const -> double
{
    return m_timer.interval(TimerTag::eTimerTagGlobal);
}

auto Engine::getCPUFrameTime() const -> double
{
    return m_frameCPUTime;
}

auto Engine::getDeviceCapture() -> DeviceCapture*
{
    return m_pDeviceCapture;
}

auto Engine::getWindowWidth() const -> uint32_t
{
    return m_pWindowSystem->getWidth();
}

auto Engine::getWindowHeight() const -> uint32_t
{
    return m_pWindowSystem->getHeight();
}

auto Engine::getPixelWidth() const -> uint32_t
{
    return m_pSwapChain->getPixelWidth();
}

auto Engine::getPixelHeight() const -> uint32_t
{
    return m_pSwapChain->getPixelHeight();
}

auto Engine::getDPIScale() const -> float
{
    return m_pWindowSystem->getDPIScale();
}

auto Engine::isHighDPIEnabled() const -> bool
{
    return m_pWindowSystem->isHighDPIEnabled();
}

auto Engine::getMaterialRegistry() const -> MaterialRegistry*
{
    return m_pMaterialRegistry;
}
} // namespace aph
