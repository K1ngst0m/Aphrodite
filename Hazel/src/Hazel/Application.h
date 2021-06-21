//
// Created by npchitman on 5/30/21.
//

#ifndef HAZEL_APPLICATION_H
#define HAZEL_APPLICATION_H

#include "Core.h"
#include "Events/Event.h"
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/LayerStack.h"
#include "Window.h"

namespace Hazel {
    class Application {
    public:
        Application();

        virtual ~Application() = default;

        void Run();

        void OnEvent(Event &e);

        void PushLayer(Layer *layer);

        void PushOverlay(Layer *layer);

        inline Window &GetWindow() { return *m_Window; }

        inline static Application &Get() { return *s_Instance; }

    private:
        bool OnWindowClose(WindowCloseEvent &e);
        bool OnWindowResize(WindowResizeEvent& e);

    private:
        std::unique_ptr<Window> m_Window;
        ImGuiLayer *m_ImGuiLayer;
        bool m_Running = true;
        bool m_Minimized = false
                ;
        LayerStack m_LayerStack;

        float m_LastFrameTime = 0.0f;

    private:
        static Application *s_Instance;
    };

    // to be defined in client
    Application *CreateApplication();
}// namespace Hazel

#endif// HAZEL_APPLICATION_H
