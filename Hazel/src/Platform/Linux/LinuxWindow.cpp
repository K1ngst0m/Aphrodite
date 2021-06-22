//
// Created by npchitman on 5/31/21.
//

#include "Platform/Linux/LinuxWindow.h"

#include "Hazel/Core/Input.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "hzpch.h"

namespace Hazel {

    static uint8_t s_GLFWWindowCount = 0;

    static void GLFWErrorCallback(int error, const char *description) {
        HZ_CORE_ERROR("GLFW Error({0}): {1}", error, description);
    }

    LinuxWindow::LinuxWindow(const WindowProps &props) {
        HZ_PROFILE_FUNCTION();

        Init(props);
    }

    LinuxWindow::~LinuxWindow() {
        HZ_PROFILE_FUNCTION();

        Shutdown();
    }

    Scope<Window> Window::Create(const WindowProps &props) {
        return CreateScope<LinuxWindow>(props);
    }

    void LinuxWindow::Init(const WindowProps &props) {
        HZ_PROFILE_FUNCTION();

        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;

        HZ_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width,
                     props.Height);

        HZ_CORE_INFO("Initializing GLFW");
        if (s_GLFWWindowCount == 0) {
            HZ_PROFILE_SCOPE("glfwInit");
            int success = glfwInit();
            HZ_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
        }

        {
            HZ_PROFILE_SCOPE("glfwCreateWindow");

#if defined(HZ_DEBUG)
            if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
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
            auto data = *(WindowData *) glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            data.EventCallback(event);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode,
                                        int action, int mods) {
            WindowData &data = *(WindowData *) glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    KeyPressedEvent event(static_cast<KeyCode>(key), 0);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(static_cast<KeyCode>(key));
                    data.EventCallback(event);
                    break;
                }
                case GLFW_REPEAT: {
                    KeyPressedEvent event(static_cast<KeyCode>(key), 1);
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
                            MouseButtonPressedEvent event(static_cast<MouseCode>(button));
                            data.EventCallback(event);
                            break;
                        }
                        case GLFW_RELEASE: {
                            MouseButtonReleasedEvent event(static_cast<MouseCode>(button));
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
        HZ_PROFILE_FUNCTION();

        glfwDestroyWindow(m_Window);

        if (--s_GLFWWindowCount == 0) {
            HZ_CORE_INFO("Terminating GLFW");
            glfwTerminate();
        }
    }

    void LinuxWindow::OnUpdate() {
        HZ_PROFILE_FUNCTION();

        glfwPollEvents();
        m_Context->SwapBuffers();
    }

    void LinuxWindow::SetVSync(bool enabled) {
        HZ_PROFILE_FUNCTION();

        glfwSwapInterval(enabled ? 1 : 0);
        m_Data.VSync = enabled;
    }

    bool LinuxWindow::IsVSync() const { return m_Data.VSync; }

}// namespace Hazel
