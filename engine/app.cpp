#include "app.h"

namespace vkl {

vklApp::vklApp(std::string sessionName)
    : m_sessionName(std::move(sessionName)) {
}

void vklApp::initWindow() {
    m_window = Window::Create();
    m_window->init(1366, 768);

    m_window->setCursorPosCallback([=](double xposIn, double yposIn) {
        this->mouseHandleDerive(xposIn, yposIn);
    });

    m_window->setFramebufferSizeCallback([=](int width, int height) {
        // this->m_framebufferResized = true;
    });

    m_window->setKeyCallback([=](int key, int scancode, int action, int mods) {
        this->keyboardHandleDerive(key, scancode, action, mods);
    });

    m_deletionQueue.push_function([=]() {
        m_window->cleanup();
    });
}

void vklApp::cleanup() {
    m_deletionQueue.flush();
}

void vklApp::initRenderer() {
    m_renderer = Renderer::Create(RenderBackend::VULKAN);
    m_renderer->setWindowData(m_window->getWindowData());
    m_renderer->init();

    m_deletionQueue.push_function([&]() {
        m_renderer->destroyDevice();
    });
}

void vklApp::mouseHandleDerive(double xposIn, double yposIn) {
    float dx = m_window->getCursorXpos() - xposIn;
    float dy = m_window->getCursorYpos() - yposIn;

    m_defaultCamera->rotate(glm::vec3(dy * m_defaultCamera->getRotationSpeed(), -dx * m_defaultCamera->getRotationSpeed(), 0.0f));
}

void vklApp::keyboardHandleDerive(int key, int scancode, int action, int mods) {
    if (action == VKL_PRESS) {
        switch (key) {
        case VKL_KEY_ESCAPE:
            m_window->close();
            break;
        case VKL_KEY_1:
            m_window->toggleCurosrVisibility();
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

    while (!m_window->shouldClose()) {
        m_window->pollEvents();
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
