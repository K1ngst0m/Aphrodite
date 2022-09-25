#include "app.h"

namespace vkl {
vklApp::vklApp(std::string sessionName, uint32_t winWidth, uint32_t winHeight)
    : m_sessionName(std::move(sessionName)), m_windowData(winWidth, winHeight),
      m_mouseData(m_windowData.width / 2.0f, m_windowData.height / 2.0f) {
}

void vklApp::initWindow() {
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_windowData.window = glfwCreateWindow(m_windowData.width, m_windowData.height, "Demo", nullptr, nullptr);
    assert(m_windowData.window);

    glfwSetWindowUserPointer(m_windowData.window, this);

    glfwSetFramebufferSizeCallback(m_windowData.window, [](GLFWwindow *window, int width, int height) {
        auto *app                 = reinterpret_cast<vklApp *>(glfwGetWindowUserPointer(window));
        app->m_windowData.width = width;
        app->m_windowData.height = height;
        app->m_framebufferResized = true;
    });
    glfwSetCursorPosCallback(m_windowData.window, [](GLFWwindow *window, double xposIn, double yposIn) {
        auto *app = reinterpret_cast<vklApp *>(glfwGetWindowUserPointer(window));
        app->mouseHandleDerive(xposIn, yposIn);
    });

    m_deletionQueue.push_function([=]() {
        glfwDestroyWindow(m_windowData.window);
        glfwTerminate();
    });
}

void vklApp::cleanup() {
    renderer->destroy();
    m_deletionQueue.flush();
}

void vklApp::initRenderer() {
    renderer = std::make_unique<VulkanRenderer>(&m_windowData);
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
    if (glfwGetKey(m_windowData.window, GLFW_KEY_1) == GLFW_PRESS) {
        if (m_mouseData.isCursorDisable) {
            glfwSetInputMode(m_windowData.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(m_windowData.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (glfwGetKey(m_windowData.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_windowData.window, true);

    if (glfwGetKey(m_windowData.window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera->keys.up = true;
    if (glfwGetKey(m_windowData.window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera->keys.down = true;
    if (glfwGetKey(m_windowData.window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera->keys.left = true;
    if (glfwGetKey(m_windowData.window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera->keys.right = true;

    if (glfwGetKey(m_windowData.window, GLFW_KEY_W) == GLFW_RELEASE)
        m_camera->keys.up = false;
    if (glfwGetKey(m_windowData.window, GLFW_KEY_S) == GLFW_RELEASE)
        m_camera->keys.down = false;
    if (glfwGetKey(m_windowData.window, GLFW_KEY_A) == GLFW_RELEASE)
        m_camera->keys.left = false;
    if (glfwGetKey(m_windowData.window, GLFW_KEY_D) == GLFW_RELEASE)
        m_camera->keys.right = false;

    m_camera->processMove(m_frameData.deltaTime);
}

void vklApp::run() {
    while (!glfwWindowShouldClose(m_windowData.window)) {
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
    initRenderer();
    initDerive();
}

} // namespace vkl
