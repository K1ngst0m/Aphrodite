#include "vulkanRenderer.h"
#include "renderData.h"
#include "sceneRenderer.h"
#include "uiRenderer.h"

#include "renderer/api/vulkan/buffer.h"
#include "renderer/api/vulkan/commandBuffer.h"
#include "renderer/api/vulkan/commandPool.h"
#include "renderer/api/vulkan/device.h"
#include "renderer/api/vulkan/framebuffer.h"
#include "renderer/api/vulkan/image.h"
#include "renderer/api/vulkan/imageView.h"
#include "renderer/api/vulkan/physicalDevice.h"
#include "renderer/api/vulkan/pipeline.h"
#include "renderer/api/vulkan/queue.h"
#include "renderer/api/vulkan/renderpass.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/api/vulkan/swapChain.h"
#include "renderer/api/vulkan/syncPrimitivesPool.h"
#include "renderer/api/vulkan/vkUtils.h"

#include "scene/mesh.h"

namespace vkl
{

const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

void VulkanRenderer::_createDefaultFramebuffers()
{
    m_fbData.framebuffers.resize(m_swapChain->getImageCount());
    m_fbData.colorImages.resize(m_swapChain->getImageCount());
    m_fbData.colorImageViews.resize(m_swapChain->getImageCount());

    // color attachment
    for(auto idx = 0; idx < m_swapChain->getImageCount(); idx++)
    {
        auto &colorImage = m_fbData.colorImages[idx];
        auto &colorImageView = m_fbData.colorImageViews[idx];

        // get swapchain image
        {
            ImageCreateInfo createInfo{
                .imageType = IMAGE_TYPE_2D,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            colorImage = m_swapChain->getImage(idx);
        }

        // get image view
        {
            ImageViewCreateInfo createInfo{
                .viewType = IMAGE_VIEW_TYPE_2D,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            m_device->createImageView(createInfo, &colorImageView, colorImage);
        }
    }

    // depth attachment
    {
        auto &depthImage = m_fbData.depthImage;
        auto &depthImageView = m_fbData.depthImageView;

        VkFormat depthFormat = m_device->getDepthFormat();

        {
            ImageCreateInfo createInfo{};
            createInfo.extent = { m_swapChain->getExtent().width, m_swapChain->getExtent().height, 1 };
            createInfo.format = static_cast<Format>(depthFormat);
            createInfo.tiling = IMAGE_TILING_OPTIMAL;
            createInfo.usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK_RESULT(m_device->createImage(createInfo, &depthImage));
        }

        VulkanCommandBuffer *cmd = m_device->beginSingleTimeCommands(m_queue.graphics);
        cmd->cmdTransitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        m_device->endSingleTimeCommands(cmd);

        {
            ImageViewCreateInfo createInfo{};
            createInfo.format = FORMAT_D32_SFLOAT;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            VK_CHECK_RESULT(m_device->createImageView(createInfo, &depthImageView, depthImage));
        }
    }

    for(uint32_t idx = 0; idx < m_swapChain->getImageCount(); idx++)
    {
        auto &framebuffer = m_fbData.framebuffers[idx];
        auto &colorAttachment = m_fbData.colorImageViews[idx];
        auto &depthAttachment = m_fbData.depthImageView;
        {
            std::vector<VulkanImageView *> attachments{ colorAttachment, depthAttachment };
            FramebufferCreateInfo createInfo{};
            createInfo.width = m_swapChain->getExtent().width;
            createInfo.height = m_swapChain->getExtent().height;
            VK_CHECK_RESULT(
                m_device->createFramebuffers(&createInfo, &framebuffer, attachments.size(), attachments.data()));
        }
    }
}

void VulkanRenderer::_createDefaultRenderPass()
{
    VkAttachmentDescription colorAttachment{
        .format = m_swapChain->getImageFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };
    VkAttachmentDescription depthAttachment{
        .format = m_device->getDepthFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    std::vector<VkAttachmentDescription> colorAttachments{
        colorAttachment,
    };

    VK_CHECK_RESULT(m_device->createRenderPass(nullptr, &m_renderPass, colorAttachments, depthAttachment));
}

void VulkanRenderer::_allocateDefaultCommandBuffers()
{
    m_device->allocateCommandBuffers(m_commandBuffers.size(), m_commandBuffers.data(), m_queue.graphics);
}

void VulkanRenderer::_createDefaultSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = vkl::init::semaphoreCreateInfo();
    VkFenceCreateInfo fenceInfo = vkl::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    m_device->getSyncPrimitiviesPool()->acquireSemaphore(m_presentSemaphore.size(), m_presentSemaphore.data());
    m_device->getSyncPrimitiviesPool()->acquireSemaphore(m_renderSemaphore.size(), m_renderSemaphore.data());

    for(uint32_t idx = 0; idx < m_config.maxFrames; idx++)
    {
        m_device->getSyncPrimitiviesPool()->acquireFence(m_inFlightFence[idx]);
    }
}

void VulkanRenderer::prepareFrame()
{
    vkWaitForFences(m_device->getHandle(), 1, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX);
    VK_CHECK_RESULT(m_swapChain->acquireNextImage(&m_imageIdx, m_renderSemaphore[m_currentFrame]));
    m_device->getSyncPrimitiviesPool()->ReleaseFence(m_inFlightFence[m_currentFrame]);
}

void VulkanRenderer::submitAndPresent()
{
    auto queue = m_queue.graphics;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderSemaphore[m_currentFrame],
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffers[m_currentFrame]->getHandle(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_presentSemaphore[m_currentFrame],
    };

    VK_CHECK_RESULT(queue->submit(1, &submitInfo, m_inFlightFence[m_currentFrame]));

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_presentSemaphore[m_currentFrame],
        .swapchainCount = 1,
        .pSwapchains = &m_swapChain->getHandle(),
        .pImageIndices = &m_imageIdx,
        .pResults = nullptr,  // Optional
    };

    VK_CHECK_RESULT(queue->present(presentInfo));

    m_currentFrame = (m_currentFrame + 1) % m_config.maxFrames;
}

void VulkanRenderer::cleanup()
{
    if(m_config.initDefaultResource)
    {
        for(auto *fb : m_fbData.framebuffers)
        {
            m_device->destroyFramebuffers(fb);
        }

        for(auto *imageView : m_fbData.colorImageViews)
        {
            m_device->destroyImageView(imageView);
        }

        m_device->destroyImageView(m_fbData.depthImageView);
        m_device->destroyImage(m_fbData.depthImage);
        m_device->destoryRenderPass(m_renderPass);
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

VulkanRenderer::VulkanRenderer(std::shared_ptr<WindowData> windowData, const RenderConfig &config) :
    Renderer(std::move(windowData), config)
{
    // create instance
    {
        volkInitialize();

        std::vector<const char *> extensions{};
        {
            uint32_t glfwExtensionCount = 0;
            const char **glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            extensions = std::vector<const char *>(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if(m_config.enableDebug)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
        }

        InstanceCreateInfo instanceCreateInfo{ .enabledExtensions = extensions };

        if(m_config.enableDebug)
        {
            instanceCreateInfo.flags = INSTANCE_CREATION_ENABLE_DEBUG;
            instanceCreateInfo.enabledLayers = validationLayers;
        }

        VK_CHECK_RESULT(VulkanInstance::Create(instanceCreateInfo, &m_instance));
    }

    // create device
    {
        DeviceCreateInfo createInfo{
            .enabledExtensions = deviceExtensions,
            // TODO select physical device
            .pPhysicalDevice = m_instance->getPhysicalDevices(0),
        };

        VK_CHECK_RESULT(VulkanDevice::Create(createInfo, &m_device));

        // get 3 type queue
        m_queue.graphics = m_device->getQueueByFlags(QUEUE_GRAPHICS);
        m_queue.compute = m_device->getQueueByFlags(QUEUE_COMPUTE);
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
        VK_CHECK_RESULT(glfwCreateWindowSurface(m_instance->getHandle(), m_windowData->window, nullptr, &m_surface));
        VK_CHECK_RESULT(m_device->createSwapchain(m_surface, &m_swapChain, m_windowData->window));
    }

    // init default resources
    if(m_config.initDefaultResource)
    {
        m_renderSemaphore.resize(m_config.maxFrames);
        m_presentSemaphore.resize(m_config.maxFrames);
        m_inFlightFence.resize(m_config.maxFrames);
        m_commandBuffers.resize(m_config.maxFrames);

        _allocateDefaultCommandBuffers();
        _createDefaultRenderPass();
        _createDefaultSyncObjects();
        _createDefaultFramebuffers();
        _createPipelineCache();
    }
}

void VulkanRenderer::_createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(m_device->getHandle(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
}
}  // namespace vkl
