//
// Created by npchitman on 5/31/21.
//

#include "LinuxWindow.h"

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "hzpch.h"

namespace Hazel {
    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char *description) {
        HZ_CORE_ERROR("GLFW Error({0}): {1}", error, description);
    }

    Window *Window::Create(const WindowProps &props) {
        return new LinuxWindow(props);
    }

}// namespace Hazel

Hazel::LinuxWindow::LinuxWindow(const Hazel::WindowProps &props) {
    Init(props);
}

Hazel::LinuxWindow::~LinuxWindow() { Shutdown(); }

void Hazel::LinuxWindow::Init(const Hazel::WindowProps &props) {
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;

    HZ_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width,
                 props.Height);

    if (!s_GLFWInitialized) {
        int success = glfwInit();
        HZ_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    m_Window = glfwCreateWindow(static_cast<int>(props.Width),
                                static_cast<int>(props.Height),
                                m_Data.Title.c_str(), nullptr, nullptr);
    m_Context = CreateScope<OpenGLContext>(m_Window);
    m_Context->Init();

    glfwSetWindowUserPointer(m_Window, &m_Data);
    SetVSync(true);

    glfwSetWindowSizeCallback(
            m_Window, [](GLFWwindow *window, int width, int height) {
                auto data = *(WindowData *) glfwGetWindowUserPointer(window);
                data.Width = width;
                data.Height = height;

                WindowResizeEvent event(width, height);
                data.EventCallback(event);
            });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window) {
        auto data = *(WindowData *) glfwGetWindowUserPointer(window);
        WindowCloseEvent event;
        data.EventCallback(event);
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode,
                                    int action, int mods) {
        WindowData &data = *(WindowData *) glfwGetWindowUserPointer(window);

        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event(key, 0);
                data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event(key);
                data.EventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event(key, 1);
                data.EventCallback(event);
                break;
            }
        }
    });

    glfwSetMouseButtonCallback(
            m_Window, [](GLFWwindow *window, int button, int action, int mods) {
                WindowData &data = *(WindowData *) glfwGetWindowUserPointer(window);

                switch (action) {
                    case GLFW_PRESS: {
                        MouseButtonPressedEvent event(button);
                        data.EventCallback(event);
                        break;
                    }
                    case GLFW_RELEASE: {
                        MouseButtonReleasedEvent event(button);
                        data.EventCallback(event);
                        break;
                    }
                }
            });

    glfwSetScrollCallback(
            m_Window, [](GLFWwindow *window, double xOffset, double yOffset) {
                WindowData &data = *(WindowData *) glfwGetWindowUserPointer(window);

                MouseScrolledEvent event((float) xOffset, (float) yOffset);
                data.EventCallback(event);
            });

    glfwSetCursorPosCallback(
            m_Window, [](GLFWwindow *window, double xPos, double yPos) {
                WindowData &data = *(WindowData *) glfwGetWindowUserPointer(window);

                MouseMovedEvent event((float) xPos, (float) yPos);
                data.EventCallback(event);
            });
}

void Hazel::LinuxWindow::Shutdown() { glfwDestroyWindow(m_Window); }

void Hazel::LinuxWindow::OnUpdate() {
    glfwPollEvents();
    m_Context->SwapBuffers();
}

void Hazel::LinuxWindow::SetVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
    m_Data.VSync = enabled;
}

bool Hazel::LinuxWindow::IsVSync() const { return m_Data.VSync; }
