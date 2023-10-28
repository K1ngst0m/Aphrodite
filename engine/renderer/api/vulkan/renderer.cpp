#include "renderer.h"
#include "api/vulkan/device.h"
#include "renderer/renderer.h"
#include "scene/mesh.h"
#include "common/common.h"

#include "common/assetManager.h"

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
        VK_LOG_ERR("%s", msg.str());
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
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
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
        m_queue[QueueType::Graphics] = m_pDevice->getQueueByFlags(QueueType::Graphics);
        m_queue[QueueType::Compute]  = m_pDevice->getQueueByFlags(QueueType::Compute);
        m_queue[QueueType::Transfer] = m_pDevice->getQueueByFlags(QueueType::Transfer);

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
        m_renderSemaphore.resize(m_config.maxFrames);
        m_frameFence.resize(m_config.maxFrames);

        {
            for(auto& semaphore : m_renderSemaphore)
            {
                semaphore = m_pDevice->acquireSemaphore();
            }
            for(auto& fence : m_frameFence)
            {
                fence = m_pDevice->acquireFence(false);
            }
        }

        // pipeline cache
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            _VR(m_pDevice->getDeviceTable()->vkCreatePipelineCache(m_pDevice->getHandle(), &pipelineCacheCreateInfo,
                                                                   vkAllocator(), &m_pipelineCache));
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
}

Renderer::~Renderer()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        delete pUI;
    }

    vkDestroyPipelineCache(m_pDevice->getHandle(), m_pipelineCache, vkAllocator());

    m_pResourceLoader->cleanup();
    m_pDevice->destroy(m_pSwapChain);
    vkDestroySurfaceKHR(m_pInstance->getHandle(), m_surface, vk::vkAllocator());
    Device::Destroy(m_pDevice.get());
    Instance::Destroy(m_pInstance);
};

void Renderer::beginFrame()
{
    {
        m_timer = std::chrono::high_resolution_clock::now();
    }
}

void Renderer::endFrame()
{
    // clean the frame data
    {
        m_pDevice->freeCommandBuffers(m_frameData.cmds.size(), m_frameData.cmds.data());
        m_frameData.cmds.clear();

        for(auto semaphore : m_frameData.semaphores)
        {
            APH_CHECK_RESULT(m_pDevice->releaseSemaphore(semaphore));
        }

        for(auto fence : m_frameData.fences)
        {
            APH_CHECK_RESULT(m_pDevice->releaseFence(fence));
        }
        m_frameData.fences.clear();
    }

    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;

    {
        m_frameCounter++;
        auto tEnd      = std::chrono::high_resolution_clock::now();
        auto tDiff     = std::chrono::duration<double, std::milli>(tEnd - m_timer).count();
        m_frameTimer   = (float)tDiff / 1000.0f;
        float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - m_lastTimestamp).count());
        if(fpsTimer > 1000.0f)
        {
            m_lastFPS       = static_cast<uint32_t>((float)m_frameCounter * (1000.0f / fpsTimer));
            m_frameCounter  = 0;
            m_lastTimestamp = tEnd;
        }
        m_tPrevEnd = tEnd;
    }
}

CommandBuffer* Renderer::acquireCommandBuffer(Queue* queue)
{
    CommandBuffer* cmd;
    APH_CHECK_RESULT(m_pDevice->allocateCommandBuffers(1, &cmd, queue));
    m_frameData.cmds.push_back(cmd);
    return cmd;
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
    m_frameData.semaphores.push_back(sem);
    return sem;
}

Fence* Renderer::acquireFence()
{
    Fence* fence = m_pDevice->acquireFence();
    m_frameData.fences.push_back(fence);
    return fence;
}

void Renderer::submit(Queue* pQueue, QueueSubmitInfo submitInfo, Image* pPresentImage)
{
    // aph::vk::QueueSubmitInfo submitInfo{.commandBuffers = cmds, .waitSemaphores = {getRenderSemaphore()}};
    Semaphore* renderSem  = {};
    Semaphore* presentSem = {};

    Fence* frameFence = m_pDevice->acquireFence(false);
    m_pDevice->getDeviceTable()->vkResetFences(m_pDevice->getHandle(), 1, &frameFence->getHandle());

    if(pPresentImage)
    {
        renderSem = m_pDevice->acquireSemaphore();
        APH_CHECK_RESULT(m_pSwapChain->acquireNextImage(renderSem->getHandle()));

        presentSem = m_pDevice->acquireSemaphore();
        submitInfo.waitSemaphores.push_back(renderSem);
        submitInfo.signalSemaphores.push_back(presentSem);
    }

    APH_CHECK_RESULT(pQueue->submit({submitInfo}, frameFence));

    if(pPresentImage)
    {
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
}  // namespace aph::vk
