#include "vulkanRenderer.h"
#include "renderData.h"
#include "sceneRenderer.h"
#include "uiRenderer.h"

#include "renderer/api/vulkan/device.h"

#include "scene/mesh.h"

namespace vkl
{

const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
        SwapChainCreateInfo createInfo{
            .surface = m_surface,
            .windowHandle = m_windowData->window,
        };
        VK_CHECK_RESULT(m_device->createSwapchain(createInfo, &m_swapChain));
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

void VulkanRenderer::_createDefaultFramebuffers()
{
    m_defaultFb.framebuffers.resize(m_swapChain->getImageCount());
    m_defaultFb.colorImages.resize(m_swapChain->getImageCount());
    m_defaultFb.colorImageViews.resize(m_swapChain->getImageCount());
    m_defaultFb.depthImages.resize(m_swapChain->getImageCount());
    m_defaultFb.depthImageViews.resize(m_swapChain->getImageCount());

    // color and depth attachment
    for(auto idx = 0; idx < m_swapChain->getImageCount(); idx++)
    {
        // color image view
        {
            auto &colorImage = m_defaultFb.colorImages[idx];
            auto &colorImageView = m_defaultFb.colorImageViews[idx];

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

        // depth image view
        {
            auto &depthImage = m_defaultFb.depthImages[idx];
            auto &depthImageView = m_defaultFb.depthImageViews[idx];
            VkFormat depthFormat = m_device->getDepthFormat();
            {
                ImageCreateInfo createInfo{
                    .extent = { m_swapChain->getExtent().width, m_swapChain->getExtent().height, 1 },
                    .usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    .property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    .format = static_cast<Format>(depthFormat),
                    .tiling = IMAGE_TILING_OPTIMAL,
                };
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

        // framebuffers
        {
            auto &framebuffer = m_defaultFb.framebuffers[idx];
            auto &colorAttachment = m_defaultFb.colorImageViews[idx];
            auto &depthAttachment = m_defaultFb.depthImageViews[idx];
            {
                std::vector<VulkanImageView *> attachments{ colorAttachment, depthAttachment };
                FramebufferCreateInfo createInfo{
                    .width = m_swapChain->getExtent().width,
                    .height = m_swapChain->getExtent().height,
                    .attachments = {attachments},
                };
                VK_CHECK_RESULT(
                    m_device->createFramebuffers(createInfo, &framebuffer));
            }
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

    RenderPassCreateInfo createInfo{
        .colorAttachments = { colorAttachment },
        .depthAttachment = { depthAttachment },
    };

    VK_CHECK_RESULT(m_device->createRenderPass(createInfo, &m_renderPass));
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
    auto *queue = m_queue.graphics;
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
        for(auto *framebuffer : m_defaultFb.framebuffers)
        {
            m_device->destroyFramebuffers(framebuffer);
        }

        for(auto *imageView : m_defaultFb.colorImageViews)
        {
            m_device->destroyImageView(imageView);
        }

        for(auto *imageView : m_defaultFb.depthImageViews)
        {
            m_device->destroyImageView(imageView);
        }

        for(auto *image : m_defaultFb.depthImages)
        {
            m_device->destroyImage(image);
        }

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

void VulkanRenderer::_createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    VK_CHECK_RESULT(vkCreatePipelineCache(m_device->getHandle(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
}
}  // namespace vkl
