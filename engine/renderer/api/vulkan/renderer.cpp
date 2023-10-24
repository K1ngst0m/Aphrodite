#include "renderer.h"
#include "api/vulkan/device.h"
#include "renderer/renderer.h"
#include "scene/mesh.h"
#include "common/common.h"

#include "api/gpuResource.h"
#include "common/assetManager.h"

#include <volk.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_glfw.h>

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
        m_queue[QueueType::GRAPHICS] = m_pDevice->getQueueByFlags(QueueType::GRAPHICS);
        m_queue[QueueType::COMPUTE]  = m_pDevice->getQueueByFlags(QueueType::COMPUTE);
        m_queue[QueueType::TRANSFER] = m_pDevice->getQueueByFlags(QueueType::TRANSFER);

        if(!m_queue[QueueType::COMPUTE])
        {
            m_queue[QueueType::COMPUTE] = m_queue[QueueType::GRAPHICS];
        }
        if(!m_queue[QueueType::TRANSFER])
        {
            m_queue[QueueType::TRANSFER] = m_queue[QueueType::COMPUTE];
        }

        // check sample count support
        {
            auto limit  = createInfo.pPhysicalDevice->getProperties()->limits;
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
        m_timelineMain.resize(m_config.maxFrames);
        m_renderSemaphore.resize(m_config.maxFrames);
        m_frameFence.resize(m_config.maxFrames);

        {
            m_pSyncPrimitivesPool = std::make_unique<SyncPrimitivesPool>(m_pDevice.get());
            m_pSyncPrimitivesPool->acquireSemaphore(m_renderSemaphore.size(), m_renderSemaphore.data());
            for(auto& fence : m_frameFence)
            {
                m_pSyncPrimitivesPool->acquireFence(fence);
            }
            vkResetFences(m_pDevice->getHandle(), m_frameFence.size(), m_frameFence.data());
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
}

Renderer::~Renderer()
{
    if(m_config.flags & RENDER_CFG_UI)
    {
        // if(m_ui.pVertexBuffer)
        // {
        //     m_pDevice->destroyBuffer(m_ui.pVertexBuffer);
        // }
        // if(m_ui.pIndexBuffer)
        // {
        //     m_pDevice->destroyBuffer(m_ui.pIndexBuffer);
        // }
        // m_pDevice->destroyImage(m_ui.pFontImage);
        // m_pDevice->destroySampler(m_ui.fontSampler);
        // m_pDevice->destroyShaderProgram(m_ui.pProgram);
        // m_pDevice->destroyPipeline(m_ui.pipeline);
        // if(ImGui::GetCurrentContext())
        // {
        //     ImGui::DestroyContext();
        // }
    }

    // TODO
    m_pSyncPrimitivesPool.reset(nullptr);

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

        m_pSyncPrimitivesPool->ReleaseSemaphores(m_frameData.semaphores.size(), m_frameData.semaphores.data());
        m_frameData.semaphores.clear();

        for(auto fence : m_frameData.fences)
        {
            m_pSyncPrimitivesPool->releaseFence(fence);
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

VkSemaphore Renderer::acquireSemahpore()
{
    VkSemaphore sem;
    m_pSyncPrimitivesPool->acquireSemaphore(1, &sem);
    m_frameData.semaphores.push_back(sem);
    return sem;
}

VkFence Renderer::acquireFence()
{
    VkFence fence;
    m_pSyncPrimitivesPool->acquireFence(fence);
    m_frameData.fences.push_back(fence);
    return fence;
}

void Renderer::submit(Queue* pQueue, QueueSubmitInfo submitInfo, Image* pPresentImage)
{
    // aph::vk::QueueSubmitInfo submitInfo{.commandBuffers = cmds, .waitSemaphores = {getRenderSemaphore()}};
    VkSemaphore renderSem  = {};
    VkSemaphore presentSem = {};

    VkFence frameFence = acquireFence();
    vkResetFences(m_pDevice->getHandle(), 1, &frameFence);

    if(pPresentImage)
    {
        renderSem = acquireSemahpore();
        _VR(m_pSwapChain->acquireNextImage(renderSem));

        presentSem = acquireSemahpore();
        submitInfo.waitSemaphores.push_back(renderSem);
        submitInfo.signalSemaphores.push_back(presentSem);
    }

    APH_CHECK_RESULT(pQueue->submit({submitInfo}, frameFence));

    if(pPresentImage)
    {
        m_pSwapChain->presentImage(pQueue, {presentSem});
    }

    _VR(vkWaitForFences(m_pDevice->getHandle(), 1, &frameFence, VK_TRUE, UINT64_MAX));
}
}  // namespace aph::vk
