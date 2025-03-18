#include "renderer.h"

#include "api/vulkan/device.h"
#include "common/common.h"
#include "common/logger.h"
#include "common/profiler.h"
#include "renderer/renderer.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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

Renderer::Renderer(const RenderConfig& config)
    : m_config(config)
{
    APH_PROFILER_SCOPE();
    WindowSystemCreateInfo wsi_create_info{
        .width = config.width,
        .height = config.height,
        .enableUI = false,
    };
    m_pWindowSystem = WindowSystem::Create(wsi_create_info);
    auto& wsi = m_pWindowSystem;
    // create instance
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        auto requiredExtensions = wsi->getRequiredExtensions();
        vk::InstanceCreateInfo instanceCreateInfo{};
#ifdef APH_DEBUG
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        instanceCreateInfo.enabledLayers.push_back("VK_LAYER_KHRONOS_validation");

        {
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
            ;
        }
#endif
        instanceCreateInfo.enabledExtensions = std::move(requiredExtensions);
        APH_VR(vk::Instance::Create(instanceCreateInfo, &m_pInstance));
    }

    // create device
    {
        uint32_t gpuIdx = 0;
        vk::DeviceCreateInfo createInfo{
            // TODO select physical device
            .enabledFeatures =
                {
                    .meshShading                = true,
                    .multiDrawIndirect          = true,
                    .tessellationSupported      = true,
                    .samplerAnisotropy = true,
                    .rayTracing                 = false,
                    .bindless = true,
                },
            .pPhysicalDevice = m_pInstance->getPhysicalDevices(gpuIdx),
            .pInstance       = m_pInstance,
        };

        m_pDevice = vk::Device::Create(createInfo);
        VK_LOG_INFO("Select Device [%d].", gpuIdx);
        APH_ASSERT(m_pDevice != nullptr);
    }

    // setup swapchain
    {
        vk::SwapChainCreateInfo createInfo{
            .pInstance = m_pInstance,
            .pWindowSystem = m_pWindowSystem.get(),
            .pQueue = m_pDevice->getQueue(QueueType::Graphics),
        };
        auto result = m_pDevice->create(createInfo, &m_pSwapChain);
        APH_ASSERT(result.success());
    }

    // init graph
    {
        m_frameGraph.resize(m_config.maxFrames);
        for (auto& graph : m_frameGraph)
        {
            graph = std::make_unique<RenderGraph>(m_pDevice.get());
        }
    }

    // init resource loader
    {
        m_pResourceLoader = std::make_unique<ResourceLoader>(ResourceLoaderCreateInfo{ .pDevice = m_pDevice.get() });
    }

    // init ui
    // {
    //     m_pUI = std::make_unique<vk::UI>(vk::UICreateInfo{
    //         .pRenderer = this,
    //         .flags     = vk::UI_Docking,
    //     });
    // }

    m_timer.set(TIMER_TAG_GLOBAL);
}

Renderer::~Renderer()
{
    APH_PROFILER_SCOPE();
    for (auto& graph : m_frameGraph)
    {
        graph.reset();
    }

    m_pResourceLoader->cleanup();
    m_pDevice->destroy(m_pSwapChain);
    vk::Device::Destroy(m_pDevice.get());
    vk::Instance::Destroy(m_pInstance);
};

void Renderer::update()
{
    APH_PROFILER_SCOPE();
    // {
    //     m_pUI->update();
    // }
}

void Renderer::unload()
{
    APH_PROFILER_SCOPE();
    // {
    //     m_pUI->unload();
    // }
};

void Renderer::load()
{
    APH_PROFILER_SCOPE();
    // {
    //     m_pUI->load();
    // }
};

void Renderer::recordGraph(std::function<void(RenderGraph*)>&& func)
{
    APH_PROFILER_SCOPE();
    for (auto& pGraph : m_frameGraph)
    {
        func(pGraph.get());
        pGraph->build(m_pSwapChain);
    }
}

void Renderer::render()
{
    APH_PROFILER_SCOPE();
    m_timer.set(TIMER_TAG_FRAME);
    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;
    // m_pDevice->begineCapture();
    m_frameGraph[m_frameIdx]->execute();
    // m_pDevice->endCapture();
    m_frameCPUTime = m_timer.interval(TIMER_TAG_FRAME);
}
} // namespace aph
