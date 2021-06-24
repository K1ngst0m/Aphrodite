//
// Created by npchitman on 5/30/21.
//

#ifndef HAZEL_APPLICATION_H
#define HAZEL_APPLICATION_H

#include "Hazel/Core/Base.h"
#include "Hazel/Core/LayerStack.h"
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/Window.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/ImGui/ImGuiLayer.h"

int main(int argc, char** argv);

namespace Hazel {
    class Application {
    public:
        Application(const std::string& name = "Hazel App");

        virtual ~Application();

        void OnEvent(Event &e);

        void PushLayer(Layer *layer);

        void PushOverlay(Layer *layer);

        Window &GetWindow() { return *m_Window; }

        void Close();

        static Application &Get() { return *s_Instance; }

    private:
        void Run();
        bool OnWindowClose(WindowCloseEvent &e);
        bool OnWindowResize(WindowResizeEvent &e);

    private:
        Scope<Window> m_Window;
        ImGuiLayer *m_ImGuiLayer;
        bool m_Running = true;
        bool m_Minimized = false;
        LayerStack m_LayerStack;

        float m_LastFrameTime = 0.0f;

    private:
        static Application *s_Instance;
        friend int ::main(int argc, char ** argv);
    };

    // to be defined in client
    Application *CreateApplication();
}// namespace Hazel

#endif// HAZEL_APPLICATION_H
