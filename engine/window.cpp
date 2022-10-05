#include "window.h"
#include <GLFW/glfw3.h>

namespace vkl {

void Window::init(uint32_t width, uint32_t height) {
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _windowData = std::make_shared<WindowData>(width, height);
    _cursorData = std::make_shared<CursorData>(width / 2.0f, height / 2.0f);

    _windowData->window = glfwCreateWindow(_windowData->width, _windowData->height, "Centimani Engine", nullptr, nullptr);
    assert(_windowData->window);
    glfwSetWindowUserPointer(getHandle(), this);
}

GLFWwindow *Window::getHandle() {
    return _windowData->window;
}
void Window::setHeight(uint32_t h) {
    _windowData->height = h;
}
void Window::setWidth(uint32_t w) {
    _windowData->width = w;
}
std::shared_ptr<WindowData> Window::getWindowData() {
    return _windowData;
}
void Window::cleanup() {
    glfwDestroyWindow(_windowData->window);
    glfwTerminate();
}
uint32_t Window::getWidth() {
    return _windowData->width;
}
uint32_t Window::getHeight() {
    return _windowData->height;
}

void Window::setFramebufferSizeCallback(const FramebufferSizeFunc &cbFunc) {
    _framebufferResizeCB = cbFunc;
    glfwSetFramebufferSizeCallback(getHandle(), [](GLFWwindow *window, int width, int height) {
        auto *ptr = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        ptr->setWidth(width);
        ptr->setHeight(height);
        ptr->_framebufferResizeCB(width, height);
    });
}

void Window::setCursorPosCallback(const CursorPosFunc &cbFunc) {
    _cursorPosCB = cbFunc;

    glfwSetCursorPosCallback(getHandle(), [](GLFWwindow *window, double xposIn, double yposIn) {
        Window *ptr = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

        auto xpos = static_cast<float>(xposIn);
        auto ypos = static_cast<float>(yposIn);

        if (ptr->_cursorData->firstMouse) {
            ptr->_cursorData->lastX      = xpos;
            ptr->_cursorData->lastY      = ypos;
            ptr->_cursorData->firstMouse = false;
        }

        ptr->_cursorPosCB(xposIn, yposIn);

        ptr->_cursorData->lastX = xpos;
        ptr->_cursorData->lastY = ypos;
    });
}

float Window::getAspectRatio() {
    return _windowData->getAspectRatio();
}

void Window::setKeyCallback(const KeyFunc &cbFunc) {
    _keyCB = cbFunc;
    glfwSetKeyCallback(getHandle(), [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto *ptr = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        ptr->_keyCB(key, scancode, action, mods);
    });
}

void Window::setCursorVisibility(bool flag) {
    glfwSetInputMode(getHandle(), GLFW_CURSOR, flag ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    _isCursorVisible = flag;
}

void Window::toggleCurosrVisibility() {
    setCursorVisibility(!_isCursorVisible);
}

void Window::close() {
    glfwSetWindowShouldClose(getHandle(), true);
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(getHandle());
}
void Window::pollEvents() {
    glfwPollEvents();
}
std::shared_ptr<CursorData> Window::getMouseData() {
    return _cursorData;
}
float Window::getCursorYpos() {
    return _cursorData->lastY;
}
float Window::getCursorXpos() {
    return _cursorData->lastX;
}
} // namespace vkl
