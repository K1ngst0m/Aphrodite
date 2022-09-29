#include "renderer/sceneRenderer.h"

#include "renderObject.h"
#include "sceneRenderer.h"
#include "uniformObject.h"
#include "vulkanRenderer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace vkl {
const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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
    m_defaultFramebuffers.resize(m_swapChain.getImageCount());
    for (size_t i = 0; i < m_defaultFramebuffers.size(); i++) {
        std::vector<VkImageView> attachments = {m_swapChain.getImageViewWithIdx(i), m_defaultDepthAttachment.view};
        m_defaultFramebuffers[i]             = m_device->createFramebuffers(m_swapChain.getExtent(), attachments, m_defaultRenderPass);
    }

    m_deletionQueue.push_function([=]() {
        for (auto &framebuffer : m_defaultFramebuffers) {
            vkDestroyFramebuffer(m_device->getLogicalDevice(), framebuffer, nullptr);
        }
    });
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

    if (m_settings.enableDebug) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanRenderer::_checkValidationLayerSupport() {
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

void VulkanRenderer::_createSurface() {
    if (glfwCreateWindowSurface(m_instance, _windowData->window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    m_deletionQueue.push_function([=]() { vkDestroySurfaceKHR(m_instance, m_surface, nullptr); });
}

void VulkanRenderer::_createInstance() {
    if (m_settings.enableDebug && !_checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    std::vector<const char *> extensions = getRequiredInstanceExtensions();
    VkInstanceCreateInfo      createInfo{
             .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
             .pApplicationInfo        = &appInfo,
             .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
             .ppEnabledExtensionNames = extensions.data(),
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);
    if (m_settings.enableDebug) {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_instance));
    m_deletionQueue.push_function([=]() { vkDestroyInstance(m_instance, nullptr); });
}

void VulkanRenderer::_createDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    assert(deviceCount > 0 && "failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    m_device = std::make_shared<VulkanDevice>(devices[0]);

    m_device->create(m_surface, m_enabledFeatures, deviceExtensions);

    vkGetDeviceQueue(m_device->getLogicalDevice(), m_device->GetQueueFamilyIndices(DeviceQueueType::GRAPHICS), 0, &graphicsQueue);
    vkGetDeviceQueue(m_device->getLogicalDevice(), m_device->GetQueueFamilyIndices(DeviceQueueType::PRESENT), 0, &presentQueue);
    vkGetDeviceQueue(m_device->getLogicalDevice(), m_device->GetQueueFamilyIndices(DeviceQueueType::TRANSFER), 0, &transferQueue);
    vkGetDeviceQueue(m_device->getLogicalDevice(), m_device->GetQueueFamilyIndices(DeviceQueueType::COMPUTE), 0, &computeQueue);

    m_deletionQueue.push_function([&]() {
        m_device->destroy();
    });
}

void VulkanRenderer::_createDefaultRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format         = m_swapChain.getFormat(),
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };
    VkAttachmentDescription depthAttachment{
        .format         = m_device->findDepthFormat(),
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

    m_defaultRenderPass = m_device->createRenderPass(colorAttachments, depthAttachment);

    m_deletionQueue.push_function([=]() {
        vkDestroyRenderPass(m_device->getLogicalDevice(), m_defaultRenderPass, nullptr);
    });
}

void VulkanRenderer::_setupSwapChain() {
    m_swapChain.create(m_device, m_surface, _windowData->window);
    m_deletionQueue.push_function([&]() {
        m_swapChain.cleanup();
    });
}

void VulkanRenderer::_recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(_windowData->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_windowData->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device->getLogicalDevice());

    m_swapChain.cleanup();
    m_swapChain.create(m_device, m_surface, _windowData->window);

    _createDefaultDepthResources();
    _createDefaultFramebuffers();
}

void VulkanRenderer::_createCommandBuffers() {
    m_defaultCommandBuffers.resize(m_swapChain.getImageCount());
    m_device->allocateCommandBuffers(m_defaultCommandBuffers.data(), m_defaultCommandBuffers.size());
}

void VulkanRenderer::_setupDebugMessenger() {
    if (!m_settings.enableDebug)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger));
    m_deletionQueue.push_function([=]() { destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr); });
}

void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                   const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::_setupPipelineBuilder() {
    m_pipelineBuilder.reset(m_swapChain.getExtent());
}

void VulkanRenderer::_createSyncObjects() {
    m_defaultSyncObjects.resize(m_settings.maxFrames);

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

void VulkanRenderer::immediateSubmit(VkQueue queue, std::function<void(VkCommandBuffer cmd)> &&function) const {
    VkCommandBuffer cmd = m_device->beginSingleTimeCommands();
    function(cmd);
    m_device->endSingleTimeCommands(cmd, queue);
}

void VulkanRenderer::prepareFrame() {
    vkWaitForFences(m_device->getLogicalDevice(), 1, &m_defaultSyncObjects[m_currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult result = m_swapChain.acqureNextImage(INT64_MAX, m_defaultSyncObjects[m_currentFrame].renderSemaphore, VK_NULL_HANDLE, &m_imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        _recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
    }

    vkResetFences(m_device->getLogicalDevice(), 1, &m_defaultSyncObjects[m_currentFrame].inFlightFence);
}
void VulkanRenderer::submitFrame() {
    VkSemaphore          waitSemaphores[]   = {m_defaultSyncObjects[m_currentFrame].renderSemaphore};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSemaphores[] = {m_defaultSyncObjects[m_currentFrame].presentSemaphore};
    VkSubmitInfo         submitInfo{
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = waitSemaphores,
                .pWaitDstStageMask    = waitStages,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &m_defaultCommandBuffers[m_imageIdx],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores    = signalSemaphores,
    };

    VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_defaultSyncObjects[m_currentFrame].inFlightFence));

    VkPresentInfoKHR presentInfo = m_swapChain.getPresentInfo(signalSemaphores, &m_imageIdx);

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _windowData->resized) {
        _windowData->resized = false;
        _recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        VK_CHECK_RESULT(result);
    }

    m_currentFrame = (m_currentFrame + 1) % m_settings.maxFrames;
}

void VulkanRenderer::initDevice() {
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
    vkDeviceWaitIdle(m_device->getLogicalDevice());
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
        .Instance       = m_instance,
        .PhysicalDevice = m_device->getPhysicalDevice(),
        .Device         = m_device->getLogicalDevice(),
        .Queue          = graphicsQueue,
        .DescriptorPool = imguiPool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };

    ImGui_ImplVulkan_Init(&initInfo, m_defaultRenderPass);

    // execute a gpu command to upload imgui font textures
    immediateSubmit(graphicsQueue, [&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

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
    if (m_settings.enableUI) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();
    }
}

void VulkanRenderer::initDefaultResource() {
    _setupPipelineBuilder();
    _createCommandBuffers();
    _createDefaultRenderPass();
    _createDefaultDepthResources();
    _createDefaultFramebuffers();
    _createSyncObjects();
}

std::shared_ptr<SceneRenderer> VulkanRenderer::getSceneRenderer() {
    if (_sceneRenderer == nullptr) {
        _sceneRenderer = std::make_shared<VulkanSceneRenderer>(this);
    }
    return _sceneRenderer;
}
VkQueue VulkanRenderer::getDeviceQueue(DeviceQueueType type) const {
    switch (type) {
    case DeviceQueueType::COMPUTE:
        return computeQueue;
    case DeviceQueueType::GRAPHICS:
        return graphicsQueue;
    case DeviceQueueType::TRANSFER:
        return transferQueue;
    case DeviceQueueType::PRESENT:
        return presentQueue;
    }
    return graphicsQueue;
}
void VulkanRenderer::recordCommandBuffer(const std::function<void()> &commands, uint32_t commandIdx) {

    commands();
}
VkRenderPass VulkanRenderer::getDefaultRenderPass() const {
    return m_defaultRenderPass;
}
VkCommandBuffer VulkanRenderer::getDefaultCommandBuffer(uint32_t idx) const {
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

void VulkanRenderer::_createDefaultDepthResources() {
    VkFormat depthFormat = m_device->findDepthFormat();
    m_device->createImage(m_swapChain.getExtent().width, m_swapChain.getExtent().height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_defaultDepthAttachment);
    m_defaultDepthAttachment.view = m_device->createImageView(m_defaultDepthAttachment.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    m_device->transitionImageLayout(transferQueue, m_defaultDepthAttachment.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    m_deletionQueue.push_function([&]() {
        m_defaultDepthAttachment.destroy();
    });
}
VkFramebuffer VulkanRenderer::getDefaultFrameBuffer(uint32_t idx) const {
    return m_defaultFramebuffers[idx];
}
} // namespace vkl
