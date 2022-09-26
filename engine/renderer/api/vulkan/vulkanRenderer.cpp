#include "renderer/sceneRenderer.h"

#include "vulkanRenderer.h"
#include "renderObject.h"
#include "sceneRenderer.h"
#include "uniformObject.h"

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

void VulkanRenderer::_createFramebuffers() {
    m_swapChain.createFramebuffers(m_defaultRenderPass);
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
    if (glfwCreateWindowSurface(m_instance, m_windowData->window, nullptr, &m_surface) != VK_SUCCESS) {
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
    m_device = new VulkanDevice(devices[0]);
    getEnabledFeatures();
    m_device->createLogicalDevice(enabledFeatures, deviceExtensions, nullptr);

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

    m_deletionQueue.push_function([=]() { delete m_device; });
}

void VulkanRenderer::_createRenderPass() {
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

    m_deletionQueue.push_function(
        [=]() { vkDestroyRenderPass(m_device->logicalDevice, m_defaultRenderPass, nullptr); });
}

void VulkanRenderer::_setupSwapChain() {
    m_swapChain.create(m_device, m_surface, m_windowData->window);
    m_deletionQueue.push_function([&](){
        m_swapChain.cleanup();
    });
}

void VulkanRenderer::_recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_windowData->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_windowData->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device->logicalDevice);
    m_swapChain.cleanup();
    m_swapChain.create(m_device, m_surface, m_windowData->window);
    m_swapChain.createDepthResources(m_queues.transfer);
    m_swapChain.createFramebuffers(m_defaultRenderPass);
}

void VulkanRenderer::_createCommandBuffers() {
    m_commandBuffers.resize(m_swapChain.getImageCount());

    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_device->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)m_commandBuffers.size(),
    };

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device->logicalDevice, &allocInfo, m_commandBuffers.data()));
}

void VulkanRenderer::_createDepthResources() {
    m_swapChain.createDepthResources(m_queues.transfer);
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
    m_pipelineBuilder.resetToDefault(m_swapChain.getExtent());
}

void VulkanRenderer::_createSyncObjects() {
    m_frameSyncObjects.resize(m_settings.maxFrames);

    VkSemaphoreCreateInfo semaphoreInfo = vkl::init::semaphoreCreateInfo();
    VkFenceCreateInfo     fenceInfo     = vkl::init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for (auto &m_frameSyncObject : m_frameSyncObjects) {
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->logicalDevice, &semaphoreInfo, nullptr, &m_frameSyncObject.presentSemaphore));
        VK_CHECK_RESULT(vkCreateSemaphore(m_device->logicalDevice, &semaphoreInfo, nullptr, &m_frameSyncObject.renderSemaphore));
        VK_CHECK_RESULT(vkCreateFence(m_device->logicalDevice, &fenceInfo, nullptr, &m_frameSyncObject.inFlightFence));

        m_deletionQueue.push_function([=]() { m_frameSyncObject.destroy(m_device->logicalDevice); });
    }
}

void VulkanRenderer::recordCommandBuffer(WindowData *windowData, const std::function<void()> &drawCommands,
                                         uint32_t frameIdx) {
    recordCommandBuffer(windowData, m_defaultRenderPass, drawCommands, frameIdx);
}

void VulkanRenderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) const {
    VkCommandBuffer cmd = m_device->beginSingleTimeCommands();
    function(cmd);
    m_device->endSingleTimeCommands(cmd, m_queues.graphics);
}

void VulkanRenderer::prepareFrame() {
    vkWaitForFences(m_device->logicalDevice, 1, &m_frameSyncObjects[m_currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    VkResult result = m_swapChain.acqureNextImage(INT64_MAX, m_frameSyncObjects[m_currentFrame].renderSemaphore, VK_NULL_HANDLE, &m_imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        _recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        VK_CHECK_RESULT(result);
    }

    vkResetFences(m_device->logicalDevice, 1, &m_frameSyncObjects[m_currentFrame].inFlightFence);
}
void VulkanRenderer::submitFrame() {
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

    VkPresentInfoKHR presentInfo = m_swapChain.getPresentInfo(signalSemaphores, &m_imageIdx);

    VkResult result = vkQueuePresentKHR(m_queues.present, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        _recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        VK_CHECK_RESULT(result);
    }

    m_currentFrame = (m_currentFrame + 1) % m_settings.maxFrames;
}

void VulkanRenderer::recordCommandBuffer(WindowData *windowData, VkRenderPass renderPass,
                                         const std::function<void()> &drawCommands,
                                         uint32_t                     frameIdx) {
    auto &commandBuffer = m_commandBuffers[frameIdx];

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    // render pass
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color                 = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil          = {1.0f, 0};
    VkRenderPassBeginInfo renderPassInfo = m_swapChain.getRenderPassBeginInfo(renderPass, clearValues, frameIdx);

    // dynamic state
    const VkViewport viewport = vkl::init::viewport(static_cast<float>(windowData->width), static_cast<float>(windowData->height));
    const VkRect2D   scissor  = vkl::init::rect2D(m_swapChain.getExtent());

    // record command
    vkResetCommandBuffer(commandBuffer, 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_settings.enableUI) {
        // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    drawCommands();

    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
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
    vkDeviceWaitIdle(m_device->logicalDevice);
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
    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(m_windowData->window, true);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo initInfo = {
        .Instance       = m_instance,
        .PhysicalDevice = m_device->physicalDevice,
        .Device         = m_device->logicalDevice,
        .Queue          = m_queues.graphics,
        .DescriptorPool = imguiPool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
    };

    ImGui_ImplVulkan_Init(&initInfo, m_defaultRenderPass);

    // execute a gpu command to upload imgui font textures
    immediateSubmit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    glfwSetKeyCallback(m_windowData->window, ImGui_ImplGlfw_KeyCallback);
    glfwSetMouseButtonCallback(m_windowData->window, ImGui_ImplGlfw_MouseButtonCallback);

    m_deletionQueue.push_function([=]() {
        vkDestroyDescriptorPool(m_device->logicalDevice, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}
void VulkanRenderer::prepareUI() {
    if (m_settings.enableUI) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();
    }
}

void VulkanRenderer::initDefaultResource() {
    _createCommandBuffers();
    _createDepthResources();
    _createRenderPass();
    _createFramebuffers();
    _setupPipelineBuilder();
    _createSyncObjects();
}

std::shared_ptr<SceneRenderer> VulkanRenderer::createSceneRenderer() {
    _sceneRenderer = std::make_shared<VulkanSceneRenderer>(this);
    return _sceneRenderer;
}
} // namespace vkl