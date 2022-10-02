#include "app.h"

namespace vkl {
vklApp::vklApp(std::string sessionName, uint32_t winWidth, uint32_t winHeight)
    : m_sessionName(std::move(sessionName)),
      m_mouseData(winWidth / 2.0f, winHeight / 2.0f) {
    m_window.init(winWidth, winHeight);
}

void vklApp::initWindow() {
    m_window.setCursorPosCallback([=](double xposIn, double yposIn) {
        this->mouseHandleDerive(xposIn, yposIn);
    });

    m_window.setFramebufferSizeCallback([=](int width, int height) {
        this->m_framebufferResized = true;
    });

    m_window.setKeyCallback([=](int key, int scancode, int action, int mods) {
        this->keyboardHandleDerive(key, scancode, action, mods);
    });

    m_deletionQueue.push_function([=]() {
        m_window.cleanup();
    });
}

void vklApp::cleanup() {
    m_deletionQueue.flush();
}

void vklApp::initRenderer() {
    m_renderer = Renderer::CreateRenderer(RenderBackend::VULKAN);
    m_renderer->setWindowData(m_window.getWindowData());
    m_renderer->initDevice();

    m_deletionQueue.push_function([&]() {
        m_renderer->destroyDevice();
    });
}

void vklApp::mouseHandleDerive(double xposIn, double yposIn) {
    // TODO remove
    return;
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

    m_defaultCamera->rotate(glm::vec3(dy * m_defaultCamera->getRotationSpeed(), -dx * m_defaultCamera->getRotationSpeed(), 0.0f));
}

void vklApp::keyboardHandleDerive(int key, int scancode, int action, int mods) {
    if (action == VKL_PRESS) {
        switch (key) {
        case VKL_KEY_ESCAPE:
            m_window.close();
            break;
        case VKL_KEY_1:
            m_window.toggleCurosrVisibility();
            break;
        case VKL_KEY_W:
            m_defaultCamera->setMovement(CameraDirection::UP, true);
            break;
        case VKL_KEY_A:
            m_defaultCamera->setMovement(CameraDirection::LEFT, true);
            break;
        case VKL_KEY_S:
            m_defaultCamera->setMovement(CameraDirection::DOWN, true);
            break;
        case VKL_KEY_D:
            m_defaultCamera->setMovement(CameraDirection::RIGHT, true);
            break;
        }
    }

    if (action == VKL_RELEASE) {
        switch (key) {
        case VKL_KEY_W:
            m_defaultCamera->setMovement(CameraDirection::UP, false);
            break;
        case VKL_KEY_A:
            m_defaultCamera->setMovement(CameraDirection::LEFT, false);
            break;
        case VKL_KEY_S:
            m_defaultCamera->setMovement(CameraDirection::DOWN, false);
            break;
        case VKL_KEY_D:
            m_defaultCamera->setMovement(CameraDirection::RIGHT, false);
            break;
        }
    }

    m_defaultCamera->processMovement(m_deltaTime);
}

void vklApp::run() {
    auto loop_start_point = std::chrono::steady_clock::now();
    while (!m_window.shouldClose()) {
        m_window.pollEvents();
        drawFrame();
    }

    m_renderer->idleDevice();
    auto loop_end_point = std::chrono::steady_clock::now();
    m_deltaTime         = (loop_end_point - loop_start_point).count();
}

void vklApp::finish() {
    cleanup();
}

void vklApp::init() {
    initWindow();
    initRenderer();
    initDerive();
}

} // namespace vkl
