#include "renderer.h"

#include <utility>
#include "sceneRenderer.h"
#include "api/vulkan/device.h"

#include "scene/mesh.h"

namespace aph
{
VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window, const RenderConfig& config) :
    IRenderer(std::move(window), config)
{
    // create instance
    {
        volkInitialize();

        std::vector<const char*> extensions{};
        {
            uint32_t     glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            extensions     = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if(m_config.enableDebug)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        InstanceCreateInfo instanceCreateInfo{ .enabledExtensions = extensions };

        if(m_config.enableDebug)
        {
            const std::vector<const char*> validationLayers = {
                "VK_LAYER_KHRONOS_validation",
            };
            instanceCreateInfo.flags         = INSTANCE_CREATION_ENABLE_DEBUG;
            instanceCreateInfo.enabledLayers = validationLayers;
        }

        VK_CHECK_RESULT(VulkanInstance::Create(instanceCreateInfo, &m_instance));
    }

    // create device
    {
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
        };

        DeviceCreateInfo createInfo{
            .enabledExtensions = deviceExtensions,
            // TODO select physical device
            .pPhysicalDevice = m_instance->getPhysicalDevices(0),
        };

        VK_CHECK_RESULT(VulkanDevice::Create(createInfo, &m_device));

        // get 3 type queue
        m_queue.graphics = m_device->getQueueByFlags(QUEUE_GRAPHICS);
        m_queue.compute  = m_device->getQueueByFlags(QUEUE_COMPUTE);
        m_queue.transfer = m_device->getQueueByFlags(QUEUE_TRANSFER);
        if(!m_queue.compute)
        {
            m_queue.compute = m_queue.graphics;
        }
        if(!m_queue.transfer)
        {
            m_queue.transfer = m_queue.compute;
        }
    }

    // setup swapchain
    {
        VK_CHECK_RESULT(glfwCreateWindowSurface(m_instance->getHandle(), m_window->getHandle(), nullptr, &m_surface));
        SwapChainCreateInfo createInfo{
            .surface      = m_surface,
            .windowHandle = m_window->getHandle(),
        };
        VK_CHECK_RESULT(m_device->createSwapchain(createInfo, &m_swapChain));
    }

    // init default resources
    if(m_config.initDefaultResource)
    {
        m_frameFences.resize(m_config.maxFrames);
        m_commandBuffers.resize(m_config.maxFrames);
        m_renderSemaphore.resize(m_config.maxFrames);
        m_presentSemaphore.resize(m_config.maxFrames);

        {
            m_pSyncPrimitivesPool = new VulkanSyncPrimitivesPool(m_device);
            m_pShaderCache        = new VulkanShaderCache(m_device);
        }

        {
            m_device->allocateCommandBuffers(m_commandBuffers.size(), m_commandBuffers.data(), m_queue.graphics);
        }

        {
            VkSemaphoreCreateInfo semaphoreInfo = aph::init::semaphoreCreateInfo();
            VkFenceCreateInfo     fenceInfo     = aph::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

            m_pSyncPrimitivesPool->acquireSemaphore(m_presentSemaphore.size(), m_presentSemaphore.data());
            m_pSyncPrimitivesPool->acquireSemaphore(m_renderSemaphore.size(), m_renderSemaphore.data());

            for(uint32_t idx = 0; idx < m_config.maxFrames; idx++)
            {
                m_pSyncPrimitivesPool->acquireFence(m_frameFences[idx]);
            }
        }

        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
            VK_CHECK_RESULT(
                vkCreatePipelineCache(m_device->getHandle(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
        }
    }
}

void VulkanRenderer::beginFrame()
{
    VK_CHECK_RESULT(m_device->waitForFence({ m_frameFences[m_frameIdx] }));
    VK_CHECK_RESULT(m_swapChain->acquireNextImage(&m_imageIdx, m_renderSemaphore[m_frameIdx]));
    VK_CHECK_RESULT(m_pSyncPrimitivesPool->releaseFence(m_frameFences[m_frameIdx]));
}

void VulkanRenderer::endFrame()
{
    auto* queue = m_queue.graphics;

    QueueSubmitInfo submitInfo{
        .commandBuffers   = { m_commandBuffers[m_frameIdx] },
        .waitStages       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
        .waitSemaphores   = { m_renderSemaphore[m_frameIdx] },
        .signalSemaphores = { m_presentSemaphore[m_frameIdx] },
    };

    VK_CHECK_RESULT(queue->submit({ submitInfo }, m_frameFences[m_frameIdx]));
    VK_CHECK_RESULT(m_swapChain->presentImage(m_imageIdx, queue, { m_presentSemaphore[m_frameIdx] }));

    m_frameIdx = (m_frameIdx + 1) % m_config.maxFrames;
}

void VulkanRenderer::cleanup()
{
    if(m_pShaderCache)
    {
        m_pShaderCache->destroy();
    }

    if(m_pSyncPrimitivesPool)
    {
        delete m_pSyncPrimitivesPool;
    }

    vkDestroyPipelineCache(m_device->getHandle(), m_pipelineCache, nullptr);

    m_device->destroySwapchain(m_swapChain);
    VulkanDevice::Destroy(m_device);
    vkDestroySurfaceKHR(m_instance->getHandle(), m_surface, nullptr);
    VulkanInstance::Destroy(m_instance);
}

void VulkanRenderer::idleDevice()
{
    m_device->waitIdle();
}

}  // namespace aph
