#include "vulkanRenderer.h"
#include "buffer.h"
#include "device.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "renderObject.h"
#include "renderer/sceneRenderer.h"
#include "renderpass.h"
#include "scene/entity.h"
#include "sceneRenderer.h"
#include "swapChain.h"
#include "uniformObject.h"
#include "vkUtils.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

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

void VulkanRenderer::_createDefaultFramebuffers() {
    m_defaultFramebuffers.resize(m_swapChain->getImageCount());
    _createDefaultColorAttachments();
    _createDefaultDepthAttachments();
    for (auto &fb : m_defaultFramebuffers) {
        {
            std::vector<VulkanImageView *> attachments{fb.colorImageView, fb.depthImageView};
            FramebufferCreateInfo          createInfo{};
            createInfo.width  = m_swapChain->getExtent().width;
            createInfo.height = m_swapChain->getExtent().height;
            VK_CHECK_RESULT(m_device->createFramebuffers(&createInfo, &fb.framebuffer, attachments.size(), attachments.data()));
        }

        m_deletionQueue.push_function([=]() {
            vkDestroyFramebuffer(m_device->getLogicalDevice(), fb.framebuffer->getHandle(m_defaultRenderPass), nullptr);
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
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
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
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance->getHandle(), &deviceCount, nullptr);
    assert(deviceCount > 0 && "failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance->getHandle(), &deviceCount, devices.data());

    m_device = std::make_shared<VulkanDevice>();

    m_device->init(devices[0], m_enabledFeatures, deviceExtensions);

    m_deletionQueue.push_function([&]() {
        m_device->destroy();
    });
}

void VulkanRenderer::_createDefaultRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format         = m_swapChain->getFormat(),
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

    VK_CHECK_RESULT(m_device->createRenderPass(nullptr, &m_defaultRenderPass, colorAttachments, depthAttachment));

    m_deletionQueue.push_function([=]() {
        m_device->destoryRenderPass(m_defaultRenderPass);
    });
}

void VulkanRenderer::_setupSwapChain() {
    m_device->createSwapchain(m_surface, &m_swapChain, _windowData.get());
    m_deletionQueue.push_function([&]() {
        m_device->destroySwapchain(m_swapChain);
    });
}

void VulkanRenderer::_createDefaultCommandBuffers() {
    m_defaultCommandBuffers.resize(m_swapChain->getImageCount());
    m_device->allocateCommandBuffers(m_defaultCommandBuffers.size(), m_defaultCommandBuffers.data());
}

void VulkanRenderer::_setupDebugMessenger() {
    if (!_config.enableDebug)
        return;

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

void VulkanRenderer::_setupPipelineBuilder() {
    m_pipelineBuilder.reset(m_swapChain->getExtent());
}

void VulkanRenderer::_createDefaultSyncObjects() {
    m_defaultSyncObjects.resize(_config.maxFrames);

    VkSemaphoreCreateInfo semaphoreInfo = vkl::init::semaphoreCreateInfo();
    VkFenceCreateInfo     fenceInfo     = vkl::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (auto &frameSyncObject : m_defaultSyncObjects) {
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &frameSyncObject.presentSemaphore));
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &frameSyncObject.renderSemaphore));
        VK_CHECK_RESULT(vkCreateFence(m_device->getLogicalDevice(), &fenceInfo, nullptr, &frameSyncObject.inFlightFence));

        m_deletionQueue.push_function([=]() {
            vkDestroyFence(m_device->getLogicalDevice(), frameSyncObject.inFlightFence, nullptr);
            vkDestroySemaphore(m_device->getLogicalDevice(), frameSyncObject.renderSemaphore, nullptr);
            vkDestroySemaphore(m_device->getLogicalDevice(), frameSyncObject.presentSemaphore, nullptr);
        });
    }
}

void VulkanRenderer::prepareFrame() {
    vkWaitForFences(m_device->getLogicalDevice(), 1, &getCurrentFrameSyncObject().inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult result = m_swapChain->acqureNextImage(getCurrentFrameSyncObject().renderSemaphore, VK_NULL_HANDLE, &m_imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        assert("swapchain recreation current not support.");
        // _recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
    }

    vkResetFences(m_device->getLogicalDevice(), 1, &getCurrentFrameSyncObject().inFlightFence);
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
                .pCommandBuffers      = &m_defaultCommandBuffers[m_imageIdx]->getHandle(),
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
        .pImageIndices      = &m_imageIdx,
        .pResults           = nullptr, // Optional
    };

    VkResult result = vkQueuePresentKHR(getDefaultDeviceQueue(QUEUE_TYPE_PRESENT), &presentInfo);
    VK_CHECK_RESULT(result);

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        _windowData->resized) {
        assert("recreate swapchain currently not support.");
        _windowData->resized = false;
        // _recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        VK_CHECK_RESULT(result);
    }

    m_currentFrame = (m_currentFrame + 1) % _config.maxFrames;
}

void VulkanRenderer::init() {
    _createInstance();
    _setupDebugMessenger();
    _createSurface();
    _createDevice();
    _setupSwapChain();
}

void VulkanRenderer::destroyDevice() {
    m_deletionQueue.flush();
}
void VulkanRenderer::idleDevice() {
    m_device->waitIdle();
}

void VulkanRenderer::initImGui() {
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000,
        .poolSizeCount = std::size(poolSizes),
        .pPoolSizes    = poolSizes,
    };

    VkDescriptorPool imguiPool;
    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->getLogicalDevice(), &poolInfo, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(_windowData->window, true);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance       = m_instance->getHandle(),
        .PhysicalDevice = m_device->getPhysicalDevice(),
        .Device         = m_device->getLogicalDevice(),
        .Queue          = getDefaultDeviceQueue(QUEUE_TYPE_GRAPHICS),
        .DescriptorPool = imguiPool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };

    ImGui_ImplVulkan_Init(&initInfo, m_defaultRenderPass->getHandle());

    // execute a gpu command to upload imgui font textures
    VulkanCommandBuffer * cmd = m_device->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd->getHandle());
    m_device->endSingleTimeCommands(cmd);

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    glfwSetKeyCallback(_windowData->window, ImGui_ImplGlfw_KeyCallback);
    glfwSetMouseButtonCallback(_windowData->window, ImGui_ImplGlfw_MouseButtonCallback);

    m_deletionQueue.push_function([=]() {
        vkDestroyDescriptorPool(m_device->getLogicalDevice(), imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}
void VulkanRenderer::prepareUIDraw() {
    if (_config.enableUI) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();
    }
}

void VulkanRenderer::initDefaultResource() {
    _setupPipelineBuilder();
    _createDefaultCommandBuffers();
    _createDefaultRenderPass();
    _createDefaultFramebuffers();
    _createDefaultSyncObjects();
}

std::shared_ptr<SceneRenderer> VulkanRenderer::getSceneRenderer() {
    if (_sceneRenderer == nullptr) {
        _sceneRenderer = std::make_shared<VulkanSceneRenderer>(this);
    }
    return _sceneRenderer;
}
VkQueue VulkanRenderer::getDefaultDeviceQueue(QueueFlags type) const {
    return m_device->getQueueByFlags(type, 0);
}
VkRenderPass VulkanRenderer::getDefaultRenderPass() const {
    return m_defaultRenderPass->getHandle();
}

VulkanCommandBuffer* VulkanRenderer::getDefaultCommandBuffer(uint32_t idx) const {
    return m_defaultCommandBuffers[idx];
}
PipelineBuilder &VulkanRenderer::getPipelineBuilder() {
    return m_pipelineBuilder;
}
uint32_t VulkanRenderer::getCommandBufferCount() const {
    return m_defaultCommandBuffers.size();
}

std::shared_ptr<VulkanDevice> VulkanRenderer::getDevice() {
    return m_device;
}

void VulkanRenderer::_createDefaultDepthAttachments() {
    for (auto &fb : m_defaultFramebuffers) {
        {
            VkFormat        depthFormat = m_device->getDepthFormat();
            ImageCreateInfo createInfo{};
            createInfo.extent   = {m_swapChain->getExtent().width, m_swapChain->getExtent().height, 1};
            createInfo.format   = static_cast<Format>(depthFormat);
            createInfo.tiling   = IMAGE_TILING_OPTIMAL;
            createInfo.usage    = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK_RESULT(m_device->createImage(&createInfo, &fb.depthImage));

            VulkanCommandBuffer* cmd = m_device->beginSingleTimeCommands(QUEUE_TYPE_TRANSFER);
            cmd->cmdTransitionImageLayout(fb.depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            m_device->endSingleTimeCommands(cmd, QUEUE_TYPE_TRANSFER);
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
VkFramebuffer VulkanRenderer::getDefaultFrameBuffer(uint32_t idx) const {
    return m_defaultFramebuffers[idx].framebuffer->getHandle(m_defaultRenderPass);
}
void VulkanRenderer::_createDefaultColorAttachments() {
    for (auto idx = 0; idx < m_swapChain->getImageCount(); idx++) {
        auto &framebuffer = m_defaultFramebuffers[idx];

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
const PerFrameSyncObject &VulkanRenderer::getCurrentFrameSyncObject() {
    return m_defaultSyncObjects[m_currentFrame];
}
} // namespace vkl
