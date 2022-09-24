#include "app.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace vkl {

void vklApp::initWindow() {
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_windowData.width, m_windowData.height, "Demo", nullptr, nullptr);
    assert(m_window);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
        auto *app                 = reinterpret_cast<vklApp *>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, double xposIn, double yposIn) {
        auto *app = reinterpret_cast<vklApp *>(glfwGetWindowUserPointer(window));
        app->mouseHandleDerive(xposIn, yposIn);
    });

    m_deletionQueue.push_function([=]() {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    });
}

void vklApp::cleanup() {
    renderer->destroy();
}

void vklApp::initVulkan() {
    renderer = std::make_unique<VulkanRenderer>();
    renderer->setWindow(m_window);
    renderer->init();
}

void vklApp::mouseHandleDerive(int xposIn, int yposIn) {
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);

    if (m_mouseData.firstMouse) {
        m_mouseData.lastX      = xpos;
        m_mouseData.lastY      = ypos;
        m_mouseData.firstMouse = false;
    }

    float dx = m_mouseData.lastX - xpos;
    float dy = m_mouseData.lastY - ypos;

    m_mouseData.lastX = xpos;
    m_mouseData.lastY = ypos;

    m_camera->rotate(glm::vec3(dy * m_camera->getRotationSpeed(), -dx * m_camera->getRotationSpeed(), 0.0f));

}

void vklApp::keyboardHandleDerive() {
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
        m_camera->keys.up = true;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera->keys.down = true;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera->keys.left = true;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera->keys.right = true;

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_RELEASE)
        m_camera->keys.up = false;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_RELEASE)
        m_camera->keys.down = false;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_RELEASE)
        m_camera->keys.left = false;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_RELEASE)
        m_camera->keys.right = false;

    m_camera->processMove(m_frameData.deltaTime);
}

vklApp::vklApp(std::string sessionName, uint32_t winWidth, uint32_t winHeight)
    : m_sessionName(std::move(sessionName)), m_windowData(winWidth, winHeight),
      m_mouseData(m_windowData.width / 2.0f, m_windowData.height / 2.0f) {
}
void vklApp::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        keyboardHandleDerive();
        drawFrame();
    }

    renderer->waitIdle();
}
void vklApp::finish() {
    cleanupDerive();
    cleanup();
}
void vklApp::init() {
    initWindow();
    initVulkan();
    initDerive();
}

// void vklApp::initImGui() {
//     // 1: create descriptor pool for IMGUI
//     //  the size of the pool is very oversize, but it's copied from imgui demo itself.
//     VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
//                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
//                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
//                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
//                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
//                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
//                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
//                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
//                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
//                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
//                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

//     VkDescriptorPoolCreateInfo poolInfo = {
//         .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//         .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
//         .maxSets       = 1000,
//         .poolSizeCount = std::size(poolSizes),
//         .pPoolSizes    = poolSizes,
//     };

//     VkDescriptorPool imguiPool;
//     VK_CHECK_RESULT(vkCreateDescriptorPool(m_device->logicalDevice, &poolInfo, nullptr, &imguiPool));

//     // 2: initialize imgui library

//     // this initializes the core structures of imgui
//     ImGui::CreateContext();

//     ImGui_ImplGlfw_InitForVulkan(m_window, true);

//     // this initializes imgui for Vulkan
//     ImGui_ImplVulkan_InitInfo initInfo = {
//         .Instance       = m_instance,
//         .PhysicalDevice = m_device->physicalDevice,
//         .Device         = m_device->logicalDevice,
//         .Queue          = m_queues.graphics,
//         .DescriptorPool = imguiPool,
//         .MinImageCount  = 3,
//         .ImageCount     = 3,
//         .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
//     };

//     ImGui_ImplVulkan_Init(&initInfo, m_defaultRenderPass);

//     // execute a gpu command to upload imgui font textures
//     immediateSubmit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

//     // clear font textures from cpu data
//     ImGui_ImplVulkan_DestroyFontUploadObjects();

//     glfwSetKeyCallback(m_window, ImGui_ImplGlfw_KeyCallback);
//     glfwSetMouseButtonCallback(m_window, ImGui_ImplGlfw_MouseButtonCallback);

//     m_deletionQueue.push_function([=]() {
//         vkDestroyDescriptorPool(m_device->logicalDevice, imguiPool, nullptr);
//         ImGui_ImplVulkan_Shutdown();
//     });
// }
// void vklApp::prepareUI() {
//     if (m_settings.enableUI) {
//         ImGui_ImplVulkan_NewFrame();
//         ImGui_ImplGlfw_NewFrame();
//         ImGui::NewFrame();
//         ImGui::ShowDemoWindow();
//         ImGui::Render();
//     }
// }

} // namespace vkl
