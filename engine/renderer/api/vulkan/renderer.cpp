#include "renderer.h"
#include "api/vulkan/device.h"
#include "renderer/renderer.h"
#include "common/common.h"
#include "common/timer.h"

#include "volk.h"

namespace aph::vk
{

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void*                                       pUserData)
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

Renderer::Renderer(WSI* wsi, const RenderConfig& config) : IRenderer(wsi, config)
{
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
        InstanceCreateInfo instanceCreateInfo{.enabledExtensions = extensions};

        if(m_config.flags & RENDER_CFG_DEBUG)
        {
            instanceCreateInfo.enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

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
        };

        m_pDevice = Device::Create(createInfo);
        VK_LOG_INFO("Select Device [%d].", gpuIdx);
        APH_ASSERT(m_pDevice != nullptr);

        // get 3 type queue
        m_queue[QueueType::Graphics] = m_pDevice->getQueue(QueueType::Graphics);
        m_queue[QueueType::Compute]  = m_pDevice->getQueue(QueueType::Compute);
        m_queue[QueueType::Transfer] = m_pDevice->getQueue(QueueType::Transfer);

        if(!m_queue[QueueType::Compute])
        {
            m_queue[QueueType::Compute] = m_queue[QueueType::Graphics];
        }
        if(!m_queue[QueueType::Transfer])
        {
            m_queue[QueueType::Transfer] = m_queue[QueueType::Compute];
        }

        // check sample count support
        {
            auto limit  = createInfo.pPhysicalDevice->getProperties().limits;
            auto counts = limit.framebufferColorSampleCounts & limit.framebufferDepthSampleCounts;
            if(!(counts & m_sampleCount))
            {
                m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
            }
        }
    }

    // setup swapchain
    {
        SwapChainCreateInfo createInfo{
            .pInstance = m_pInstance,
            .pWsi      = m_wsi,
        };
        auto result = m_pDevice->create(createInfo, &m_pSwapChain);
        APH_ASSERT(result.success());
    }

    // init default resources
    if(m_config.flags & RENDER_CFG_DEFAULT_RES)
    {
        // pipeline cache
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            _VR(m_pDevice->getDeviceTable()->vkCreatePipelineCache(m_pDevice->getHandle(), &pipelineCacheCreateInfo,
                                                                   vkAllocator(), &m_pipelineCache));
        }
    }

    // init frame data
    {
        m_frameData.resize(m_config.maxFrames);
        for(auto& frameData : m_frameData)
        {
            frameData.renderSemaphore = m_pDevice->acquireSemaphore();
            frameData.fence           = m_pDevice->acquireFence(false);

            // TODO
            VkQueryPoolCreateInfo createInfo{
                .sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                .pNext      = nullptr,
                .flags      = 0,
                .queryType  = VK_QUERY_TYPE_TIMESTAMP,
                .queryCount = 2,
            };

            m_pDevice->getDeviceTable()->vkCreateQueryPool(m_pDevice->getHandle(), &createInfo, vkAllocator(),
                                                           &frameData.queryPool);
            // TODO
            // m_pDevice->getDeviceTable()->vkResetQueryPool(m_pDevice->getHandle(), frameData.queryPool, 0, 2);
        }
    }

    // init resource loader
    {
        m_pResourceLoader = std::make_unique<ResourceLoader>(ResourceLoaderCreateInfo{.pDevice = m_pDevice.get()});
    }

    // init ui
    if(m_config.flags & RENDER_CFG_UI)
    {
        pUI = new UI({
            .pRenderer = this,
            .flags     = UI_Docking,
        });
    }

    // init query pool
    {
    }
}

Renderer::~Renderer()
{
    resetFrameData();
    for(auto& frameData : m_frameData)
    {
        m_pDevice->getDeviceTable()->vkDestroyQueryPool(m_pDevice->getHandle(), frameData.queryPool, vkAllocator());
    }

    if(m_config.flags & RENDER_CFG_UI)
    {
        delete pUI;
    }

    m_pDevice->getDeviceTable()->vkDestroyPipelineCache(m_pDevice->getHandle(), m_pipelineCache, vkAllocator());

    m_pResourceLoader->cleanup();
    m_pDevice->destroy(m_pSwapChain);
    vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, vk::vkAllocator());
    Device::Destroy(m_pDevice.get());
    Instance::Destroy(m_pInstance);
};

void Renderer::nextFrame()
{
    resetFrameData();
    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;
}

CommandPool* Renderer::acquireCommandPool(Queue* queue, bool transient)
{
    CommandPool* cmdPool = m_pDevice->acquireCommandPool({queue, transient});
    m_frameData[m_frameIdx].cmdPools.push_back(cmdPool);
    return cmdPool;
}

Shader* Renderer::getShaders(const std::filesystem::path& path) const
{
    Shader* shader = {};
    m_pResourceLoader->load({.data = path}, &shader);
    return shader;
}

Semaphore* Renderer::acquireSemahpore()
{
    Semaphore* sem = m_pDevice->acquireSemaphore();
    m_frameData[m_frameIdx].semaphores.push_back(sem);
    return sem;
}

Fence* Renderer::acquireFence()
{
    Fence* fence = m_pDevice->acquireFence();
    m_frameData[m_frameIdx].fences.push_back(fence);
    return fence;
}

void Renderer::submit(Queue* pQueue, QueueSubmitInfo submitInfo, Image* pPresentImage)
{
    Semaphore* renderSem  = {};
    Semaphore* presentSem = {};

    Fence* frameFence = acquireFence();
    m_pDevice->getDeviceTable()->vkResetFences(m_pDevice->getHandle(), 1, &frameFence->getHandle());

    if(pPresentImage)
    {
        renderSem = acquireSemahpore();
        APH_CHECK_RESULT(m_pSwapChain->acquireNextImage(renderSem->getHandle()));

        presentSem = acquireSemahpore();
        submitInfo.waitSemaphores.push_back(renderSem);
        submitInfo.signalSemaphores.push_back(presentSem);
    }

    APH_CHECK_RESULT(pQueue->submit({submitInfo}, frameFence));

    if(pPresentImage)
    {
        auto pSwapchainImage = m_pSwapChain->getImage();
        executeSingleCommands(pQueue, [&](CommandBuffer* cmd) {
            cmd->transitionImageLayout(pPresentImage, RESOURCE_STATE_COPY_SRC);
            cmd->transitionImageLayout(pSwapchainImage, RESOURCE_STATE_COPY_DST);

            if(pPresentImage->getWidth() == pSwapchainImage->getWidth() &&
               pPresentImage->getHeight() == pSwapchainImage->getHeight() &&
               pPresentImage->getDepth() == pSwapchainImage->getDepth())
            {
                VK_LOG_DEBUG("copy image to swapchain.");
                cmd->copyImage(pPresentImage, pSwapchainImage);
            }
            else
            {
                VK_LOG_DEBUG("blit image to swapchain.");
                cmd->blitImage(pPresentImage, pSwapchainImage);
            }

            cmd->transitionImageLayout(pSwapchainImage, RESOURCE_STATE_PRESENT);
        });

        APH_CHECK_RESULT(m_pSwapChain->presentImage(pQueue, {presentSem}));
    }

    frameFence->wait();
}

void Renderer::update(float deltaTime)
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        pUI->update();
    }
}

void Renderer::unload()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        pUI->unload();
    }
};
void Renderer::load()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        pUI->load();
    }
};
void Renderer::resetFrameData()
{
    // clean the frame data
    auto& frameData = m_frameData[m_frameIdx];
    {
        for(auto pool : frameData.cmdPools)
        {
            pool->reset();
            APH_CHECK_RESULT(m_pDevice->releaseCommandPool(pool));
        }
        frameData.cmdPools.clear();

        for(auto semaphore : frameData.semaphores)
        {
            APH_CHECK_RESULT(m_pDevice->releaseSemaphore(semaphore));
        }
        frameData.semaphores.clear();

        for(auto fence : frameData.fences)
        {
            APH_CHECK_RESULT(m_pDevice->releaseFence(fence));
        }
        frameData.fences.clear();

        // TODO
        // m_pDevice->getDeviceTable()->vkResetQueryPool(m_pDevice->getHandle(), frameData.queryPool, 0, 2);
    }
}
void Renderer::executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func)
{
    auto*          commandPool = acquireCommandPool(queue, true);
    CommandBuffer* cmd         = nullptr;
    APH_CHECK_RESULT(commandPool->allocate(1, &cmd));

    _VR(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
    func(cmd);
    _VR(cmd->end());

    QueueSubmitInfo submitInfo{.commandBuffers = {cmd}};
    auto            fence = m_pDevice->acquireFence();
    APH_CHECK_RESULT(queue->submit({submitInfo}, fence));
    fence->wait();

    commandPool->free(1, &cmd);
}
}  // namespace aph::vk
