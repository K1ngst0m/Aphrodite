//
// Created by npchitman on 5/30/21.
//

#include "hzpch.h"
#include "Application.h"

#include "Hazel/Log.h"

#include <glad/glad.h>

#include "Input.h"

namespace Hazel {
    Application *Application::s_Instance = nullptr;
}

Hazel::Application::Application() {
    HZ_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));

    m_ImGuiLayer = new ImGuiLayer();
    PushOverlay(m_ImGuiLayer);

}

Hazel::Application::~Application() = default;

void Hazel::Application::OnEvent(Hazel::Event &e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::OnWindowClose));

    for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) {
        (*--it)->OnEvent(e);
        if (e.Handled)
            break;
    }
}

[[noreturn]] void Hazel::Application::Run() {

    while (m_Running) {
        glClearColor(0.5, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        for(const auto & layer: m_LayerStack)
            layer->OnUpdate();

        m_ImGuiLayer->Begin();
        for(const auto & layer: m_LayerStack)
            layer->OnImGuiRender();
        m_ImGuiLayer->End();

        m_Window->OnUpdate();
    }
}

bool Hazel::Application::OnWindowClose(Hazel::WindowCloseEvent &e) {
    m_Running = false;
    return true;
}

void Hazel::Application::PushLayer(Hazel::Layer *layer) {
    m_LayerStack.PushLayer(layer);
}

void Hazel::Application::PushOverlay(Hazel::Layer *layer) {
    m_LayerStack.PushOverlay(layer);
}
