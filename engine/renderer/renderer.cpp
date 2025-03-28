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

    m_timer.set(TIMER_TAG_GLOBAL);

    // create window system
    {
        WindowSystemCreateInfo wsi_create_info{
            .width = config.width,
            .height = config.height,
            .enableUI = false,
        };
        m_pWindowSystem = WindowSystem::Create(wsi_create_info);
    }

    // create instance
    {
        APH_PROFILER_SCOPE();
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        auto requiredExtensions = m_pWindowSystem->getRequiredExtensions();
        vk::InstanceCreateInfo instanceCreateInfo{};
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

    auto postDeviceGroup = m_taskManager.createTaskGroup("post device object creation");

    {
        vk::SwapChainCreateInfo createInfo{
            .pInstance = m_pInstance,
            .pWindowSystem = m_pWindowSystem.get(),
            .pQueue = m_pDevice->getQueue(QueueType::Graphics),
        };

        postDeviceGroup->addTask(
            [](const vk::SwapChainCreateInfo& createInfo, vk::SwapChain** ppSwapchain, vk::Device* pDevice) -> TaskType
            {
                auto result = pDevice->create(createInfo, ppSwapchain);
                co_return result;
            }(createInfo, &m_pSwapChain, m_pDevice.get()));

        m_frameGraph.resize(m_config.maxFrames);

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
            [](std::unique_ptr<ResourceLoader>& resourceLoader, vk::Device* pDevice) -> TaskType
            {
                ResourceLoaderCreateInfo createInfo{ .async = true, .pDevice = pDevice };
                resourceLoader = std::make_unique<ResourceLoader>(createInfo);
                if (!resourceLoader)
                {
                    co_return { Result::RuntimeError, "Failed to initialized resource loader." };
                }
                co_return Result::Success;
            }(m_pResourceLoader, m_pDevice.get()));

        APH_VR(postDeviceGroup->submit());
    }

    // init ui
    // {
    //     m_pUI = std::make_unique<vk::UI>(vk::UICreateInfo{
    //         .pRenderer = this,
    //         .flags     = vk::UI_Docking,
    //     });
    // }
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

coro::generator<RenderGraph*> Renderer::recordGraph()
{
    APH_PROFILER_SCOPE();
    auto group = m_taskManager.createTaskGroup("record render graph");
    for (auto& pGraph : m_frameGraph)
    {
        co_yield pGraph.get();
        group->addTask(
            [](vk::SwapChain* swapchain, RenderGraph* graph) -> TaskType
            {
                graph->build(swapchain);
                co_return { Result::Success };
            }(m_pSwapChain, pGraph.get()));
    }
    APH_VR(group->submit());
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
