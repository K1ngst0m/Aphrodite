#include "vulkanRenderer.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "device.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "renderObject.h"
#include "renderer/api/vulkan/physicalDevice.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/sceneRenderer.h"
#include "renderpass.h"
#include "scene/entity.h"
#include "sceneRenderer.h"
#include "swapChain.h"
#include "uiRenderer.h"
#include "uniformObject.h"
#include "vkUtils.h"

namespace vkl {
const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static bool _checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void                                       *pUserData) {

    switch (messageSeverity) {
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

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT    *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo                 = {};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

std::unique_ptr<VulkanRenderer> VulkanRenderer::Create(RenderConfig *config, std::shared_ptr<WindowData> windowData) {
    auto instance = std::make_unique<VulkanRenderer>(std::move(windowData), config);
    return instance;
}

void VulkanRenderer::_createDefaultFramebuffers() {
    m_defaultResource.framebuffers.resize(m_swapChain->getImageCount());
    _createDefaultColorAttachments();
    _createDefaultDepthAttachments();
    for (auto &fb : m_defaultResource.framebuffers) {
        {
            std::vector<VulkanImageView *> attachments{fb.colorImageView, fb.depthImageView};
            FramebufferCreateInfo          createInfo{};
            createInfo.width  = m_swapChain->getExtent().width;
            createInfo.height = m_swapChain->getExtent().height;
            VK_CHECK_RESULT(m_device->createFramebuffers(&createInfo, &fb.framebuffer, attachments.size(), attachments.data()));
        }

        m_deletionQueue.push_function([=]() {
            m_device->destroyFramebuffers(fb.framebuffer);
        });
    }
}

std::vector<const char *> VulkanRenderer::getRequiredInstanceExtensions() {
    // Get extensions supported by the instance and store for later use
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
            for (VkExtensionProperties extension : extensions) {
                m_supportedInstanceExtensions.push_back(extension.extensionName);
            }
        }
    }

    uint32_t     glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (_config.enableDebug) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanRenderer::_createSurface() {
    if (glfwCreateWindowSurface(m_instance->getHandle(), _windowData->window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    m_deletionQueue.push_function([=]() { vkDestroySurfaceKHR(m_instance->getHandle(), m_surface, nullptr); });
}

void VulkanRenderer::_createInstance() {
    if (_config.enableDebug && !_checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    std::vector<const char *> extensions = getRequiredInstanceExtensions();

    VkApplicationInfo appInfo{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Centimani",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "Centimani Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    InstanceCreateInfo instanceCreateInfo{
        .pNext                   = nullptr,
        .pApplicationInfo        = &appInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);
    if (_config.enableDebug) {
        instanceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        instanceCreateInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
    }

    VulkanInstance::Create(&instanceCreateInfo, &m_instance);
    m_deletionQueue.push_function([=]() {
        VulkanInstance::Destroy(m_instance);
    });
}

void VulkanRenderer::_createDevice() {
    DeviceCreateInfo createInfo{};
    createInfo.enabledLayerCount       = validationLayers.size();
    createInfo.ppEnabledLayerNames     = validationLayers.data();
    createInfo.enabledExtensionCount   = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VK_CHECK_RESULT(VulkanDevice::Create(m_instance->getPhysicalDevices(0), &createInfo, &m_device));

    m_deletionQueue.push_function([&]() {
        VulkanDevice::Destroy(m_device);
    });
}

void VulkanRenderer::_createDefaultRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format         = m_swapChain->getImageFormat(),
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };
    VkAttachmentDescription depthAttachment{
        .format         = m_device->getDepthFormat(),
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    std::vector<VkAttachmentDescription> colorAttachments{
        colorAttachment,
    };

    VK_CHECK_RESULT(m_device->createRenderPass(nullptr, &m_defaultResource.renderPass, colorAttachments, depthAttachment));

    m_deletionQueue.push_function([=]() {
        m_device->destoryRenderPass(m_defaultResource.renderPass);
    });
}

void VulkanRenderer::_setupSwapChain() {
    m_device->createSwapchain(m_surface, &m_swapChain, _windowData.get());
    m_deletionQueue.push_function([&]() {
        m_device->destroySwapchain(m_swapChain);
    });
}

void VulkanRenderer::_allocateDefaultCommandBuffers() {
    m_defaultResource.commandBuffers.resize(m_swapChain->getImageCount());
    m_device->allocateCommandBuffers(m_defaultResource.commandBuffers.size(), m_defaultResource.commandBuffers.data());
}

void VulkanRenderer::_setupDebugMessenger() {
    if (!_config.enableDebug) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(m_instance->getHandle(), &createInfo, nullptr, &m_debugMessenger));
    m_deletionQueue.push_function([=]() { destroyDebugUtilsMessengerEXT(m_instance->getHandle(), m_debugMessenger, nullptr); });
}

void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                   const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::_createDefaultSyncObjects() {
    m_defaultResource.syncObjects.resize(_config.maxFrames);

    VkSemaphoreCreateInfo semaphoreInfo = vkl::init::semaphoreCreateInfo();
    VkFenceCreateInfo     fenceInfo     = vkl::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (auto &frameSyncObject : m_defaultResource.syncObjects) {
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->getHandle(), &semaphoreInfo, nullptr, &frameSyncObject.presentSemaphore));
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->getHandle(), &semaphoreInfo, nullptr, &frameSyncObject.renderSemaphore));
        VK_CHECK_RESULT(vkCreateFence(m_device->getHandle(), &fenceInfo, nullptr, &frameSyncObject.inFlightFence));

        m_deletionQueue.push_function([=]() {
            vkDestroyFence(m_device->getHandle(), frameSyncObject.inFlightFence, nullptr);
            vkDestroySemaphore(m_device->getHandle(), frameSyncObject.renderSemaphore, nullptr);
            vkDestroySemaphore(m_device->getHandle(), frameSyncObject.presentSemaphore, nullptr);
        });
    }
}

void VulkanRenderer::prepareFrame() {
    vkWaitForFences(m_device->getHandle(), 1, &getCurrentFrameSyncObject().inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult result = m_swapChain->acqureNextImage(getCurrentFrameSyncObject().renderSemaphore, VK_NULL_HANDLE, &getCurrentFrameSyncObject().imageIdx);

    // if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    //     assert("swapchain recreation current not support.");
    //     // _recreateSwapChain();
    //     return;
    // }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
    }

    vkResetFences(m_device->getHandle(), 1, &getCurrentFrameSyncObject().inFlightFence);
}

void VulkanRenderer::submitFrame() {
    VkSemaphore          waitSemaphores[]   = {getCurrentFrameSyncObject().renderSemaphore};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSemaphores[] = {getCurrentFrameSyncObject().presentSemaphore};
    VkSubmitInfo         submitInfo{
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = waitSemaphores,
                .pWaitDstStageMask    = waitStages,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &m_defaultResource.commandBuffers[getCurrentFrameSyncObject().imageIdx]->getHandle(),
                .signalSemaphoreCount = 1,
                .pSignalSemaphores    = signalSemaphores,
    };

    VK_CHECK_RESULT(vkQueueSubmit(getDefaultDeviceQueue(QUEUE_TYPE_GRAPHICS),
                                  1, &submitInfo,
                                  getCurrentFrameSyncObject().inFlightFence));

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = signalSemaphores,
        .swapchainCount     = 1,
        .pSwapchains        = &m_swapChain->getHandle(),
        .pImageIndices      = &getCurrentFrameSyncObject().imageIdx,
        .pResults           = nullptr, // Optional
    };

    VkResult result = vkQueuePresentKHR(getDefaultDeviceQueue(QUEUE_TYPE_PRESENT), &presentInfo);
    VK_CHECK_RESULT(result);

    // if (result == VK_ERROR_OUT_OF_DATE_KHR ||
    //     result == VK_SUBOPTIMAL_KHR ||
    //     _windowData->resized) {
    //     assert("recreate swapchain currently not support.");
    //     _windowData->resized = false;
    //     // _recreateSwapChain();
    // } else if (result != VK_SUCCESS) {
    //     VK_CHECK_RESULT(result);
    // }

    m_currentFrame = (m_currentFrame + 1) % _config.maxFrames;
}

void VulkanRenderer::cleanup() {
    m_shaderCache.destory(m_device->getHandle());
    m_deletionQueue.flush();
}

void VulkanRenderer::idleDevice() {
    m_device->waitIdle();
}

void VulkanRenderer::_initDefaultResource() {
    _allocateDefaultCommandBuffers();
    _createDefaultRenderPass();
    _createDefaultFramebuffers();
    _createDefaultSyncObjects();
    _createPipelineCache();
    _setupDemoPass();
}

VkQueue VulkanRenderer::getDefaultDeviceQueue(QueueFamilyType type) const {
    return m_device->getQueueByFlags(type, 0);
}
VulkanRenderPass *VulkanRenderer::getDefaultRenderPass() const {
    return m_defaultResource.renderPass;
}

VulkanCommandBuffer *VulkanRenderer::getDefaultCommandBuffer(uint32_t idx) const {
    return m_defaultResource.commandBuffers[idx];
}

uint32_t VulkanRenderer::getCommandBufferCount() const {
    return m_defaultResource.commandBuffers.size();
}

VulkanDevice *VulkanRenderer::getDevice() const {
    return m_device;
}

void VulkanRenderer::_createDefaultDepthAttachments() {
    for (auto &fb : m_defaultResource.framebuffers) {
        {
            VkFormat        depthFormat = m_device->getDepthFormat();
            ImageCreateInfo createInfo{};
            createInfo.extent   = {m_swapChain->getExtent().width, m_swapChain->getExtent().height, 1};
            createInfo.format   = static_cast<Format>(depthFormat);
            createInfo.tiling   = IMAGE_TILING_OPTIMAL;
            createInfo.usage    = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK_RESULT(m_device->createImage(&createInfo, &fb.depthImage));

            VulkanCommandBuffer *cmd = m_device->beginSingleTimeCommands(QUEUE_TYPE_TRANSFER);
            cmd->cmdTransitionImageLayout(fb.depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            m_device->endSingleTimeCommands(cmd);
        }

        {
            ImageViewCreateInfo createInfo{};
            createInfo.format   = FORMAT_D32_SFLOAT;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            VK_CHECK_RESULT(m_device->createImageView(&createInfo, &fb.depthImageView, fb.depthImage));
        }

        m_deletionQueue.push_function([=]() {
            m_device->destroyImage(fb.depthImage);
            m_device->destroyImageView(fb.depthImageView);
        });
    }
}
VulkanFramebuffer *VulkanRenderer::getDefaultFrameBuffer(uint32_t idx) const {
    return m_defaultResource.framebuffers[idx].framebuffer;
}
void VulkanRenderer::_createDefaultColorAttachments() {
    for (auto idx = 0; idx < m_swapChain->getImageCount(); idx++) {
        auto &framebuffer = m_defaultResource.framebuffers[idx];

        // get swapchain image
        {
            framebuffer.colorImage = m_swapChain->getImage(idx);
        }

        // get image view
        {
            ImageViewCreateInfo createInfo{};
            createInfo.format   = FORMAT_B8G8R8A8_SRGB;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            m_device->createImageView(&createInfo, &framebuffer.colorImageView, framebuffer.colorImage);
        }

        m_deletionQueue.push_function([=]() {
            m_device->destroyImageView(framebuffer.colorImageView);
        });
    }
}
PerFrameSyncObject &VulkanRenderer::getCurrentFrameSyncObject() {
    return m_defaultResource.syncObjects[m_currentFrame];
}
VulkanInstance *VulkanRenderer::getInstance() const {
    return m_instance;
}

void VulkanRenderer::drawDemo() {
    VkExtent2D extent{
        .width  = getWindowWidth(),
        .height = getWindowHeight(),
    };
    VkViewport viewport = vkl::init::viewport(extent);
    VkRect2D   scissor  = vkl::init::rect2D(extent);

    RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.pRenderPass       = getDefaultRenderPass();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = extent;
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color                = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil         = {1.0f, 0};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues    = clearValues.data();

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // record command
    for (uint32_t commandIndex = 0; commandIndex < getCommandBufferCount(); commandIndex++) {
        auto *commandBuffer = getDefaultCommandBuffer(commandIndex);

        commandBuffer->begin(0);

        // render pass
        renderPassBeginInfo.pFramebuffer = getDefaultFrameBuffer(commandIndex);
        commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);

        // dynamic state
        commandBuffer->cmdSetViewport(&viewport);
        commandBuffer->cmdSetSissor(&scissor);
        commandBuffer->cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_defaultResource.demoPipeline);
        commandBuffer->cmdDraw(3, 1, 0, 0);
        commandBuffer->cmdEndRenderPass();

        commandBuffer->end();
    }
}
VulkanShaderCache &VulkanRenderer::getShaderCache() {
    return m_shaderCache;
}
void VulkanRenderer::renderOneFrame() {
    prepareFrame();
    submitFrame();
}
VulkanRenderer::VulkanRenderer(std::shared_ptr<WindowData> windowData, RenderConfig *config)
    : Renderer(std::move(windowData), config) {
    _createInstance();
    _setupDebugMessenger();
    _createSurface();
    _createDevice();
    _setupSwapChain();
    if (_config.initDefaultResource) {
        _initDefaultResource();
    }
}

void VulkanRenderer::_setupDemoPass() {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    PipelineCreateInfo createInfo{};
    createInfo._vertexInputInfo = vertexInputInfo;

    // build Shader
    std::filesystem::path shaderDir = "assets/shaders/glsl/default";

    EffectInfo info{};
    info.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] = m_shaderCache.getShaders(m_device, shaderDir / "triangle.vert.spv");
    info.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = m_shaderCache.getShaders(m_device, shaderDir / "triangle.frag.spv");
    m_defaultResource.demoEffect = ShaderEffect::Create(m_device, &info);

    VK_CHECK_RESULT(m_device->createGraphicsPipeline(&createInfo, m_defaultResource.demoEffect, getDefaultRenderPass(), &m_defaultResource.demoPipeline));

    m_deletionQueue.push_function([=]() {
        delete m_defaultResource.demoEffect;
        delete m_defaultResource.demoPipeline;
    });
}

VkExtent2D VulkanRenderer::getSwapChainExtent() const {
    return m_swapChain->getExtent();
}

void VulkanRenderer::_createPipelineCache() {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(m_device->getHandle(), &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));
}

VkPipelineCache VulkanRenderer::getPipelineCache() {
    return m_pipelineCache;
}
} // namespace vkl
