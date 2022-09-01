#include "vklBase.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkl {

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void                                       *pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}


void vklBase::createFramebuffers() {
    m_framebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {m_swapChainImageViews[i], m_depthAttachment.view};

        VkFramebufferCreateInfo framebufferInfo{
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = m_defaultRenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments    = attachments.data(),
            .width           = m_swapChainExtent.width,
            .height          = m_swapChainExtent.height,
            .layers          = 1,
        };

        VK_CHECK_RESULT(vkCreateFramebuffer(m_device->logicalDevice, &framebufferInfo, nullptr, &m_framebuffers[i]));
    }
}

std::vector<const char *> vklBase::getRequiredInstanceExtensions() {
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

    if (m_settings.isEnableValidationLayer) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool vklBase::checkValidationLayerSupport() {
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

void vklBase::createSurface() {
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    m_deletionQueue.push_function([=](){
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    });
}

void vklBase::createInstance() {
    if (m_settings.isEnableValidationLayer && !checkValidationLayerSupport()) {
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
    if (m_settings.isEnableValidationLayer) {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_instance));
    m_deletionQueue.push_function([=](){
        vkDestroyInstance(m_instance, nullptr);
    });
}

void vklBase::initWindow() {
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_windowData.width, m_windowData.height, "Demo", nullptr, nullptr);
    assert(m_window);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
        auto *app                 = reinterpret_cast<vklBase *>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, double xposIn, double yposIn) {
        auto *app = reinterpret_cast<vklBase *>(glfwGetWindowUserPointer(window));
        app->mouseHandleDerive(xposIn, yposIn);
    });

    m_deletionQueue.push_function([=](){
        glfwDestroyWindow(m_window);
        glfwTerminate();
    });
}

void vklBase::createDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    assert(deviceCount > 0 && "failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    m_device = new Device(devices[0]);
    getEnabledFeatures();
    m_device->createLogicalDevice(m_device->features, deviceExtensions, nullptr);

    VkBool32                presentSupport = false;
    std::optional<uint32_t> presentQueueFamilyIndices;
    uint32_t                i = 0;
    for (const auto &queueFamily : m_device->queueFamilyProperties) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_device->physicalDevice, i, m_surface, &presentSupport);
        if (presentSupport) {
            presentQueueFamilyIndices = i;
            break;
        }
        i++;
    }
    assert(presentQueueFamilyIndices.has_value());
    m_device->queueFamilyIndices.present = presentQueueFamilyIndices.value();

    vkGetDeviceQueue(m_device->logicalDevice, m_device->queueFamilyIndices.graphics, 0, &m_queues.graphics);
    vkGetDeviceQueue(m_device->logicalDevice, m_device->queueFamilyIndices.present, 0, &m_queues.present);
    vkGetDeviceQueue(m_device->logicalDevice, m_device->queueFamilyIndices.transfer, 0, &m_queues.transfer);

    m_deletionQueue.push_function([=](){
        delete m_device;
    });
}

void vklBase::createRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format         = m_swapChainImageFormat,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass{
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkSubpassDependency dependency{
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo                 renderPassInfo{
                        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                        .attachmentCount = static_cast<uint32_t>(attachments.size()),
                        .pAttachments    = attachments.data(),
                        .subpassCount    = 1,
                        .pSubpasses      = &subpass,
                        .dependencyCount = 1,
                        .pDependencies   = &dependency,
    };

    VK_CHECK_RESULT(vkCreateRenderPass(m_device->logicalDevice, &renderPassInfo, nullptr, &m_defaultRenderPass));

    m_deletionQueue.push_function([=](){
        vkDestroyRenderPass(m_device->logicalDevice, m_defaultRenderPass, nullptr);
    });
}

void vklBase::cleanup() {
    cleanupSwapChain();

    m_deletionQueue.flush();
}

void vklBase::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    createDevice();
    createSwapChain();
    createSwapChainImageViews();
    createCommandBuffers();
    createDepthResources();
    createRenderPass();
    createFramebuffers();
    setupPipelineBuilder();
    createSyncObjects();
}

void vklBase::createSwapChainImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        m_swapChainImageViews[i] = m_device->createImageView(m_swapChainImages[i], m_swapChainImageFormat);
    }
}

void vklBase::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_device->physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = vkl::utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR   presentMode   = vkl::utils::chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D         extent        = vkl::utils::chooseSwapExtent(swapChainSupport.capabilities, m_window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,

        .surface = m_surface,

        .minImageCount    = imageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

        .preTransform   = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode    = presentMode,
        .clipped        = VK_TRUE,
        .oldSwapchain   = VK_NULL_HANDLE,
    };

    std::array<uint32_t, 2> queueFamilyIndices = {m_device->queueFamilyIndices.graphics,
                                                  m_device->queueFamilyIndices.present};

    if (m_device->queueFamilyIndices.graphics != m_device->queueFamilyIndices.present) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device->logicalDevice, &createInfo, nullptr, &m_swapChain));

    vkGetSwapchainImagesKHR(m_device->logicalDevice, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device->logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent      = extent;
}

void vklBase::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device->logicalDevice);

    cleanupSwapChain();

    createSwapChain();
    createSwapChainImageViews();
    createDepthResources();
    createFramebuffers();
}

void vklBase::cleanupSwapChain() {
    m_depthAttachment.destroy();

    for (auto &m_swapChainFramebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_device->logicalDevice, m_swapChainFramebuffer, nullptr);
    }

    for (auto &m_swapChainImageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device->logicalDevice, m_swapChainImageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device->logicalDevice, m_swapChain, nullptr);
}

void vklBase::createCommandBuffers() {
    m_commandBuffers.resize(m_swapChainImages.size());

    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_device->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)m_commandBuffers.size(),
    };

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device->logicalDevice, &allocInfo, m_commandBuffers.data()));
}

void vklBase::loadImageFromFile(vkl::Texture &texture, std::string_view imagePath) {
    int          texWidth, texHeight, texChannels;
    stbi_uc     *pixels    = stbi_load(imagePath.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    assert(pixels && "read texture failed.");

    vkl::Buffer stagingBuffer;
    m_device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    stagingBuffer.map();
    stagingBuffer.copyTo(pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    m_device->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture);

    m_device->transitionImageLayout(m_queues.graphics, texture.image, VK_FORMAT_R8G8B8A8_SRGB,
                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    m_device->copyBufferToImage(m_queues.graphics, stagingBuffer.buffer, texture.image, static_cast<uint32_t>(texWidth),
                                static_cast<uint32_t>(texHeight));
    m_device->transitionImageLayout(m_queues.graphics, texture.image, VK_FORMAT_R8G8B8A8_SRGB,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.destroy();
}

void vklBase::createDepthResources() {
    VkFormat depthFormat = m_device->findDepthFormat();
    m_device->createImage(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_depthAttachment);
    m_depthAttachment.view = m_device->createImageView(m_depthAttachment.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    m_device->transitionImageLayout(m_queues.graphics, m_depthAttachment.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

SwapChainSupportDetails vklBase::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}
void vklBase::mouseHandleDerive(int xposIn, int yposIn) {
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);

    if (m_mouseData.firstMouse) {
        m_mouseData.lastX      = xpos;
        m_mouseData.lastY      = ypos;
        m_mouseData.firstMouse = false;
    }

    float xoffset = xpos - m_mouseData.lastX;
    float yoffset = m_mouseData.lastY - ypos;

    m_mouseData.lastX = xpos;
    m_mouseData.lastY = ypos;

    m_camera.ProcessMouseMovement(xoffset, yoffset);
}
void vklBase::keyboardHandleDerive() {
    if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS) {
        if (m_mouseData.isCursorDisable) {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera.move(CameraMovementEnum::FORWARD, m_frameData.deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera.move(CameraMovementEnum::BACKWARD, m_frameData.deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera.move(CameraMovementEnum::LEFT, m_frameData.deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera.move(CameraMovementEnum::RIGHT, m_frameData.deltaTime);
}
vklBase::vklBase(std::string sessionName, uint32_t winWidth, uint32_t winHeight)
    : m_sessionName(std::move(sessionName)), m_windowData(winWidth, winHeight),
      m_mouseData(m_windowData.width / 2.0f, m_windowData.height / 2.0f),
      m_camera(Camera((float)m_windowData.width / m_windowData.height))

{
}
void vklBase::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        keyboardHandleDerive();

        drawFrame();
    }

    vkDeviceWaitIdle(m_device->logicalDevice);
}
void vklBase::finish() {
    cleanupDerive();
    cleanup();
}
void vklBase::init() {
    initWindow();
    initVulkan();
    initDerive();
}
void vklBase::prepareFrame() {
    float currentFrame    = glfwGetTime();
    m_frameData.deltaTime = currentFrame - m_frameData.lastFrame;
    m_frameData.lastFrame = currentFrame;

    vkWaitForFences(m_device->logicalDevice, 1, &m_frameSyncObjects[m_currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult result =
        vkAcquireNextImageKHR(m_device->logicalDevice, m_swapChain, UINT64_MAX,
                              m_frameSyncObjects[m_currentFrame].renderSemaphore, VK_NULL_HANDLE, &m_imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
    }

    vkResetFences(m_device->logicalDevice, 1, &m_frameSyncObjects[m_currentFrame].inFlightFence);
}
void vklBase::submitFrame() {
    VkSemaphore          waitSemaphores[]   = {m_frameSyncObjects[m_currentFrame].renderSemaphore};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSemaphores[] = {m_frameSyncObjects[m_currentFrame].presentSemaphore};
    VkSubmitInfo         submitInfo{
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = waitSemaphores,
                .pWaitDstStageMask    = waitStages,
                .commandBufferCount   = 1,
                .pCommandBuffers      = &m_commandBuffers[m_imageIdx],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores    = signalSemaphores,
    };

    VK_CHECK_RESULT(vkQueueSubmit(m_queues.graphics, 1, &submitInfo, m_frameSyncObjects[m_currentFrame].inFlightFence));

    VkSwapchainKHR swapChains[] = {m_swapChain};

    VkPresentInfoKHR presentInfo{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = signalSemaphores,
        .swapchainCount     = 1,
        .pSwapchains        = swapChains,
        .pImageIndices      = &m_imageIdx,
        .pResults           = nullptr, // Optional
    };

    VkResult result = vkQueuePresentKHR(m_queues.present, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        VK_CHECK_RESULT(result);
    }

    m_currentFrame = (m_currentFrame + 1) % m_settings.max_frames;
}
void vklBase::createSyncObjects() {
    m_frameSyncObjects.resize(m_settings.max_frames);

    VkSemaphoreCreateInfo semaphoreInfo = vkl::init::semaphoreCreateInfo();
    VkFenceCreateInfo fenceInfo = vkl::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (auto &m_frameSyncObject : m_frameSyncObjects) {
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->logicalDevice, &semaphoreInfo, nullptr, &m_frameSyncObject.presentSemaphore));
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->logicalDevice, &semaphoreInfo, nullptr, &m_frameSyncObject.renderSemaphore));
        VK_CHECK_RESULT(vkCreateFence(m_device->logicalDevice, &fenceInfo, nullptr, &m_frameSyncObject.inFlightFence));

        m_deletionQueue.push_function([=](){
            m_frameSyncObject.destroy(m_device->logicalDevice);
        });
    }
}

void vklBase::setupPipelineBuilder() {
    vkl::VertexLayout::setPipelineVertexInputState({vkl::VertexComponent::POSITION, vkl::VertexComponent::NORMAL,
                                                    vkl::VertexComponent::UV, vkl::VertexComponent::COLOR});
    m_pipelineBuilder._vertexInputInfo = vkl::VertexLayout::_pipelineVertexInputStateCreateInfo;
    m_pipelineBuilder._inputAssembly =
        vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    m_pipelineBuilder._viewport =
        vkl::init::viewport(static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height));
    m_pipelineBuilder._scissor = vkl::init::rect2D(m_swapChainExtent);

    m_pipelineBuilder._dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    m_pipelineBuilder._dynamicState  = vkl::init::pipelineDynamicStateCreateInfo(
         m_pipelineBuilder._dynamicStages.data(), static_cast<uint32_t>(m_pipelineBuilder._dynamicStages.size()));

    m_pipelineBuilder._rasterizer = vkl::init::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    m_pipelineBuilder._multisampling        = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    m_pipelineBuilder._colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        VK_FALSE);
    m_pipelineBuilder._depthStencil =
        vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
}
void vklBase::recordCommandBuffer(const std::function<void(VkCommandBuffer cmdBuffer)> &drawCommands, uint32_t frameIdx) {
    auto &commandBuffer = m_commandBuffers[frameIdx];

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // render pass
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    VkRenderPassBeginInfo renderPassInfo =
        vkl::init::renderPassBeginInfo(m_defaultRenderPass, clearValues, m_framebuffers[frameIdx]);
    renderPassInfo.renderArea = {
        .offset = {0, 0},
        .extent = m_swapChainExtent,
    };

    // dynamic state
    const VkViewport viewport =
        vkl::init::viewport(static_cast<float>(m_windowData.width), static_cast<float>(m_windowData.height));
    const VkRect2D scissor = vkl::init::rect2D(m_swapChainExtent);

    // record command
    vkResetCommandBuffer(commandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    drawCommands(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}

void vklBase::setupDebugMessenger() {
    if (!m_settings.isEnableValidationLayer)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger));
    m_deletionQueue.push_function([=](){
        destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    });
}

void vklBase::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                            const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void vklBase::immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) {
    VkCommandBuffer cmd = m_device->beginSingleTimeCommands();
    function(cmd);
    m_device->endSingleTimeCommands(cmd, m_queues.graphics);
}

} // namespace vkl
