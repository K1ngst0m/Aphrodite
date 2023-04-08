#include "window.h"
#include <GLFW/glfw3.h>

namespace aph
{
std::shared_ptr<Window> Window::Create(uint32_t width, uint32_t height)
{
    auto instance = std::make_shared<Window>(width, height);
    return instance;
}

Window::Window(uint32_t width, uint32_t height)
{
    m_windowData = std::make_shared<WindowData>(width, height);
    m_cursorData = std::make_shared<CursorData>(width / 2.0f, height / 2.0f);
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_windowData->window =
        glfwCreateWindow(m_windowData->width, m_windowData->height, "Aphrodite Engine", nullptr, nullptr);
    assert(m_windowData->window);
    glfwSetWindowUserPointer(getHandle(), this);
}

Window::~Window()
{
    glfwDestroyWindow(m_windowData->window);
    glfwTerminate();
}

void Window::setFramebufferSizeCallback(const FramebufferSizeFunc &cbFunc)
{
    m_framebufferResizeCB = cbFunc;
    glfwSetFramebufferSizeCallback(getHandle(), [](GLFWwindow *window, int width, int height) {
        auto *ptr = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        ptr->setWidth(width);
        ptr->setHeight(height);
        ptr->m_framebufferResizeCB(width, height);
    });
}

void Window::setCursorPosCallback(const CursorPosFunc &cbFunc)
{
    m_cursorPosCB = cbFunc;

    glfwSetCursorPosCallback(getHandle(), [](GLFWwindow *window, double xposIn, double yposIn) {
        Window *ptr = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));

        auto xpos{ static_cast<float>(xposIn) };
        auto ypos{ static_cast<float>(yposIn) };

        if(ptr->m_cursorData->firstMouse)
        {
            ptr->m_cursorData->lastX = xpos;
            ptr->m_cursorData->lastY = ypos;
            ptr->m_cursorData->firstMouse = false;
        }

        ptr->m_cursorPosCB(xposIn, yposIn);

        ptr->m_cursorData->lastX = xpos;
        ptr->m_cursorData->lastY = ypos;
    });
}

void Window::setKeyCallback(const KeyFunc &cbFunc)
{
    m_keyCB = cbFunc;
    glfwSetKeyCallback(getHandle(), [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto *ptr = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        ptr->m_keyCB(key, scancode, action, mods);
    });
}

void Window::setCursorVisibility(bool flag)
{
    glfwSetInputMode(getHandle(), GLFW_CURSOR, flag ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    m_cursorData->isCursorVisible = flag;
}

void Window::close()
{
    glfwSetWindowShouldClose(getHandle(), true);
}
bool Window::shouldClose()
{
    return glfwWindowShouldClose(getHandle());
}
void Window::pollEvents()
{
    glfwPollEvents();
}
}  // namespace aph
