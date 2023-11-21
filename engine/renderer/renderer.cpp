#include "renderer.h"
#include "api/vulkan/device.h"
#include "renderer/renderer.h"
#include "common/common.h"
#include "common/logger.h"

#include "volk.h"

namespace aph::vk
{

[[maybe_unused]] static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    static std::mutex errMutex;  // Mutex for thread safety
    static uint32_t   errCount = 0;

    std::stringstream msg;
    if(messageType != VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        uint32_t frameId = *(uint32_t*)pUserData;
        msg << "[fr:" << frameId << "] ";
    }

    for(uint32_t idx = 0; idx < pCallbackData->objectCount; idx++)
    {
        auto& obj = pCallbackData->pObjects[idx];
        if(obj.pObjectName)
        {
            msg << "[name: " << obj.pObjectName << "]";
        }
    }

    msg << " >>> ";

    msg << pCallbackData->pMessage;

    switch(messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        VK_LOG_DEBUG("%s", msg.str());
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        VK_LOG_INFO("%s", msg.str());
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        VK_LOG_WARN("%s", msg.str());
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    {
        std::lock_guard<std::mutex> lock(errMutex);
        if(++errCount > 10)
        {
            VK_LOG_ERR("Too many errors, exit.");
            throw aph::TracedException();
        }
        VK_LOG_ERR("%s", msg.str().c_str());
    }
    break;

    default:
        break;
    }
    return VK_FALSE;
}

Renderer::Renderer(const RenderConfig& config) : m_config(config)
{
    m_wsi     = WSI::Create({config.width, config.height, (config.flags & RENDER_CFG_UI) != 0});
    auto& wsi = m_wsi;
    // create instance
    {
        volkInitialize();

        auto extensions = wsi->getRequiredExtensions();
#ifdef APH_DEBUG
        if(m_config.flags & RENDER_CFG_DEBUG)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        // extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        // extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        InstanceCreateInfo instanceCreateInfo{.enabledExtensions = extensions};

        if(m_config.flags & RENDER_CFG_DEBUG)
        {
            instanceCreateInfo.enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

#ifdef APH_DEBUG
        if(m_config.flags & RENDER_CFG_DEBUG)
        {
            auto& debugInfo           = instanceCreateInfo.debugCreateInfo;
            debugInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
            debugInfo.pfnUserCallback = debugCallback;
            debugInfo.pUserData       = &m_frameIdx;
        }
#endif

        _VR(Instance::Create(instanceCreateInfo, &m_pInstance));
    }

    // create device
    {
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,  VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
            VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
        };

        uint32_t         gpuIdx = 0;
        DeviceCreateInfo createInfo{
            .enabledExtensions = deviceExtensions,
            // TODO select physical device
            .pPhysicalDevice = m_pInstance->getPhysicalDevices(gpuIdx),
            .pInstance       = m_pInstance,
        };

        m_pDevice = Device::Create(createInfo);
        VK_LOG_INFO("Select Device [%d].", gpuIdx);
        APH_ASSERT(m_pDevice != nullptr);
    }

    // setup swapchain
    {
        SwapChainCreateInfo createInfo{
            .pInstance = m_pInstance,
            .pWsi      = m_wsi.get(),
        };
        auto result = m_pDevice->create(createInfo, &m_pSwapChain);
        APH_ASSERT(result.success());
    }

    // init graph
    {
        m_frameGraph.resize(m_config.maxFrames);
        m_frameFence.resize(m_config.maxFrames);
        for(auto& graph : m_frameGraph)
        {
            graph = std::make_unique<RenderGraph>(m_pDevice.get());
        }
        for(auto& fence : m_frameFence)
        {
            fence = m_pDevice->acquireFence(true);
        }
    }

    // init resource loader
    {
        m_pResourceLoader = std::make_unique<ResourceLoader>(ResourceLoaderCreateInfo{.pDevice = m_pDevice.get()});
    }

    // init ui
    if(m_config.flags & RENDER_CFG_UI)
    {
        m_pUI = std::make_unique<UI>(UICreateInfo{
            .pRenderer = this,
            .flags     = UI_Docking,
        });
    }
}

Renderer::~Renderer()
{
    for(auto& graph : m_frameGraph)
    {
        graph.reset();
    }

    m_pResourceLoader->cleanup();
    m_pDevice->destroy(m_pSwapChain);
    vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, vk::vkAllocator());
    Device::Destroy(m_pDevice.get());
    Instance::Destroy(m_pInstance);
};

void Renderer::update()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        m_pUI->update();
    }
}

void Renderer::unload()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        m_pUI->unload();
    }
};
void Renderer::load()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        m_pUI->load();
    }
};
void Renderer::recordGraph(std::function<void(RenderGraph*)>&& func)
{
    for(auto& pGraph : m_frameGraph)
    {
        auto taskGroup = m_taskManager.createTaskGroup("frame graph recording");
        taskGroup->addTask([this, &pGraph, func]() {
            func(pGraph.get());
            pGraph->build(m_pSwapChain);
        });
        m_taskManager.submit(taskGroup);
    }
    m_taskManager.wait();
}
void Renderer::render()
{
    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;
    m_frameFence[m_frameIdx]->wait();
    m_frameGraph[m_frameIdx]->execute(m_frameFence[m_frameIdx]);
}
}  // namespace aph::vk
