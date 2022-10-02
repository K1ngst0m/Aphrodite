#include "window.h"
#include <GLFW/glfw3.h>

namespace vkl {

void Window::init(uint32_t width, uint32_t height) {
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _data = std::make_shared<WindowData>(width, height);

    _data->window = glfwCreateWindow(_data->width, _data->height, "Demo", nullptr, nullptr);
    assert(_data->window);
    glfwSetWindowUserPointer(getHandle(), this);
}

GLFWwindow *Window::getHandle() {
    return _data->window;
}
void Window::setHeight(uint32_t h) {
    _data->height = h;
}
void Window::setWidth(uint32_t w) {
    _data->width = w;
}
std::shared_ptr<WindowData> Window::getWindowData() {
    return _data;
}
void Window::cleanup() {
    glfwDestroyWindow(_data->window);
    glfwTerminate();
}
uint32_t Window::getWidth() {
    return _data->width;
}
uint32_t Window::getHeight() {
    return _data->height;
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
        ptr->_cursorPosCB(xposIn, yposIn);
    });
}

float Window::getAspectRatio() {
    return _data->getAspectRatio();
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
    isCursorVisible = flag;
}

void Window::toggleCurosrVisibility() {
    setCursorVisibility(!isCursorVisible);
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
} // namespace vkl
