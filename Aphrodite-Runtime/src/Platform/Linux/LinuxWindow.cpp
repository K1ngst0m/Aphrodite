//
// Created by npchitman on 5/31/21.
//

#include "Platform/Linux/LinuxWindow.h"

#include "Aphrodite/Events/ApplicationEvent.h"
#include "Aphrodite/Events/KeyEvent.h"
#include "Aphrodite/Events/MouseEvent.h"
#include "Aphrodite/Input/Input.h"
#include "Aphrodite/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "pch.h"

namespace Aph {

    static uint8_t s_GLFWWindowCount = 0;

    static void GLFWErrorCallback(int error, const char *description) {
        APH_CORE_ERROR("GLFW Error({0}): {1}", error, description);
    }

    LinuxWindow::LinuxWindow(const WindowProps &props) {
        APH_PROFILE_FUNCTION();

        Init(props);
    }

    LinuxWindow::~LinuxWindow() {
        APH_PROFILE_FUNCTION();

        Shutdown();
    }

    void LinuxWindow::Init(const WindowProps &props) {
        APH_PROFILE_FUNCTION();

        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        APH_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

        if (s_GLFWWindowCount == 0) {
            APH_PROFILE_SCOPE("glfwInit");
            int success = glfwInit();
            APH_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        {
            APH_PROFILE_SCOPE("glfwCreateWindow");
            m_Window = glfwCreateWindow(static_cast<int>(props.Width),
                                        static_cast<int>(props.Height),
                                        m_Data.Title.c_str(), nullptr, nullptr);
            ++s_GLFWWindowCount;
        }

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
            auto &data = *(WindowData *) glfwGetWindowUserPointer(window);

            WindowCloseEvent event;
            data.EventCallback(event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode,
                                        int action, int mods) {
            auto &data = *(WindowData *) glfwGetWindowUserPointer(window);

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

        glfwSetCharCallback(m_Window, [](GLFWwindow *window, unsigned int keycode) {
            WindowData &data = *(WindowData *) glfwGetWindowUserPointer(window);

            KeyTypedEvent event(keycode);
            data.EventCallback(event);
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

    void LinuxWindow::Shutdown() {
        APH_PROFILE_FUNCTION();

        glfwDestroyWindow(m_Window);

        if (--s_GLFWWindowCount == 0) {
            APH_CORE_INFO("Terminating GLFW");
            glfwTerminate();
        }
    }

    void LinuxWindow::OnUpdate() {
        APH_PROFILE_FUNCTION();

        glfwPollEvents();
        m_Context->SwapBuffers();
    }

    void LinuxWindow::SetVSync(bool enabled) {
        APH_PROFILE_FUNCTION();

        glfwSwapInterval(enabled ? 1 : 0);
        m_Data.VSync = enabled;
    }

    bool LinuxWindow::IsVSync() const { return m_Data.VSync; }

}// namespace Aph
