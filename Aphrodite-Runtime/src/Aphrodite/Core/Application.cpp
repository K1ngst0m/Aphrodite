//
// Created by npchitman on 5/30/21.
//

#include "Application.h"

#include <GLFW/glfw3.h>

#include <memory>

#include "Aphrodite/Debug/Log.h"
#include "Aphrodite/Input/Input.h"
#include "Aphrodite/Renderer/Renderer.h"
#include "pch.h"

namespace Aph {
    Application *Application::s_Instance = nullptr;

    Application::Application(const std::string &name, ApplicationCommandLineArgs args) : m_CommandLineArgs(args) {
        APH_PROFILE_FUNCTION();
        APH_CORE_ASSERT(!s_Instance, "Application already exists!");

        s_Instance = this;
        m_Window = Window::Create(WindowProps(name));
        m_Window->SetEventCallback(APH_BIND_EVENT_FN(OnEvent));

        Renderer::Init();

        m_ImGuiLayer = new UILayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application() {
        APH_PROFILE_FUNCTION();

        Renderer::Shutdown();
    }

    void Application::OnEvent(Event &e) {
        APH_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(APH_BIND_EVENT_FN(OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(APH_BIND_EVENT_FN(OnWindowResize));

        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); it++) {
            if (e.Handled) break;
            (*it)->OnEvent(e);
        }
    }

    void Application::Run() {
        APH_PROFILE_FUNCTION();

        while (m_Running) {
            APH_PROFILE_SCOPE("RunLoop");

            auto time = static_cast<float>(glfwGetTime());
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (!m_Minimized) {
                {
                    APH_PROFILE_SCOPE("LayerStack OnUpdate");
                    for (const auto &layer : m_LayerStack)
                        layer->OnUpdate(timestep);
                }

                Aph::UILayer::Begin();
                {
                    APH_PROFILE_SCOPE("LayerStack OnUpdate");
                    for (const auto &layer : m_LayerStack)
                        layer->OnUIRender();
                }
                Aph::UILayer::End();
            }

            m_Window->OnUpdate();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent &e) {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent &e) {
        APH_PROFILE_FUNCTION();

        if (e.GetWidth() == 0 || e.GetHeight() == 0) {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return true;
    }

    void Application::PushLayer(Layer *layer) {
        APH_PROFILE_FUNCTION();

        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *layer) {
        APH_PROFILE_FUNCTION();

        m_LayerStack.PushOverlay(layer);
        layer->OnAttach();
    }

    void Application::Close() {
        m_Running = false;
    }

}// namespace Aph-Runtime
