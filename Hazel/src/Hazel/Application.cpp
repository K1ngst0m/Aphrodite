//
// Created by Npchitman on 2021/1/17.
//

#include "Application.h"

#include "ApplicationEvent.h"
#include "Hazel/Log.h"

namespace Hazel{
    #define BIND_EVENT_FN(x) std::bind(&Application::OnEvent, this, std::placeholders::_1)
    Application::Application(){
        m_Window = std::unique_ptr<Window>(Window::Create());
        m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
    }

    Application::~Application() = default;

    void Application::OnEvent(Event &e) {

    }

    void Application::Run() const {
        while(m_Running){
            glClearColor(1, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            m_Window->OnUpdate();
        }
    }
}
