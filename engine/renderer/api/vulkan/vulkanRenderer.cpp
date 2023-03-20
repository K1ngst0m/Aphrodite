#include "vulkanRenderer.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "device.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "physicalDevice.h"
#include "pipeline.h"
#include "queue.h"
#include "renderData.h"
#include "renderer/sceneRenderer.h"
#include "renderpass.h"
#include "scene/mesh.h"
#include "sceneRenderer.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitivesPool.h"
#include "uiRenderer.h"
#include "vkUtils.h"

namespace vkl
{

namespace
{
const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
bool checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for(const auto &layerProperties : availableLayers)
        {
            if(strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if(!layerFound)
        {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                             void *pUserData)
{
    switch(messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        std::cerr << "[DEBUG] >>> " << pCallbackData->pMessage << std::endl;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        std::cerr << "[INFO] >>> " << pCallbackData->pMessage << std::endl;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        std::cerr << "[WARNING] >>> " << pCallbackData->pMessage << std::endl;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        std::cerr << "[ERROR] >>> " << pCallbackData->pMessage << std::endl;
        break;
    default:
        break;
    }
    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

std::vector<const char *> getRequiredInstanceExtensions(bool isEnableDebug)
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(isEnableDebug)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

}  // namespace

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

        VulkanCommandBuffer *cmd = m_device->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
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
            VK_CHECK_RESULT(m_device->createFramebuffers(&createInfo, &framebuffer, attachments.size(),
                                                         attachments.data()));
        }
    }
}

void VulkanRenderer::_createSurface()
{
    if(glfwCreateWindowSurface(m_instance->getHandle(), _windowData->window, nullptr, &m_surface) !=
       VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void VulkanRenderer::_createInstance()
{
    if(_config.enableDebug && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    std::vector<const char *> extensions = getRequiredInstanceExtensions(_config.enableDebug);

    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Centimani",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Centimani Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    InstanceCreateInfo instanceCreateInfo{
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);
    if(_config.enableDebug)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
    }

    VulkanInstance::Create(&instanceCreateInfo, &m_instance);
}

void VulkanRenderer::_createDevice()
{
    DeviceCreateInfo createInfo{};
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VK_CHECK_RESULT(VulkanDevice::Create(m_instance->getPhysicalDevices(0), &createInfo, &m_device));
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

    VK_CHECK_RESULT(
        m_device->createRenderPass(nullptr, &m_renderPass, colorAttachments, depthAttachment));
}

void VulkanRenderer::_setupSwapChain()
{
    m_device->createSwapchain(m_surface, &m_swapChain, _windowData.get());
}

void VulkanRenderer::_allocateDefaultCommandBuffers()
{
    m_device->allocateCommandBuffers(m_commandBuffers.size(), m_commandBuffers.data());
}

void VulkanRenderer::_setupDebugMessenger()
{
    if(!_config.enableDebug)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(createDebugUtilsMessengerEXT(m_instance->getHandle(), &createInfo, nullptr, &m_debugMessenger));
}

void VulkanRenderer::_createDefaultSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo = vkl::init::semaphoreCreateInfo();
    VkFenceCreateInfo fenceInfo = vkl::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    m_device->getSyncPrimitiviesPool()->acquireSemaphore(m_presentSemaphore.size(),
                                                         m_presentSemaphore.data());
    m_device->getSyncPrimitiviesPool()->acquireSemaphore(m_renderSemaphore.size(),
                                                         m_renderSemaphore.data());

    for(uint32_t idx = 0; idx < _config.maxFrames; idx++)
    {
        m_device->getSyncPrimitiviesPool()->acquireFence(&m_inFlightFence[idx]);
    }
}

void VulkanRenderer::prepareFrame()
{
    vkWaitForFences(m_device->getHandle(), 1, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX);
    VkResult result = m_swapChain->acquireNextImage(&m_imageIdx, m_renderSemaphore[m_currentFrame]);
    m_device->getSyncPrimitiviesPool()->ReleaseFence(m_inFlightFence[m_currentFrame]);
}

void VulkanRenderer::submitAndPresent()
{
    auto queue = m_device->getQueueByFlags(VK_QUEUE_GRAPHICS_BIT);
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

    m_currentFrame = (m_currentFrame + 1) % _config.maxFrames;
}

void VulkanRenderer::cleanup()
{
    if(_config.initDefaultResource)
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
    destroyDebugUtilsMessengerEXT(m_instance->getHandle(), m_debugMessenger, nullptr);
    VulkanInstance::Destroy(m_instance);
}

void VulkanRenderer::idleDevice()
{
    m_device->waitIdle();
}

VulkanRenderer::VulkanRenderer(std::shared_ptr<WindowData> windowData, const RenderConfig &config) :
    Renderer(std::move(windowData), config)
{
    volkInitialize();
    _createInstance();
    _setupDebugMessenger();
    _createSurface();
    _createDevice();
    _setupSwapChain();
    if(_config.initDefaultResource)
    {
        m_renderSemaphore.resize(_config.maxFrames);
        m_presentSemaphore.resize(_config.maxFrames);
        m_inFlightFence.resize(_config.maxFrames);
        m_commandBuffers.resize(_config.maxFrames);

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
    VK_CHECK_RESULT(vkCreatePipelineCache(m_device->getHandle(), &pipelineCacheCreateInfo, nullptr,
                                          &m_pipelineCache));
}
}  // namespace vkl
