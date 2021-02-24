//
// Created by Npchitman on 2021/1/17.
//

#ifndef HAZELENGINE_APPLICATION_H
#define HAZELENGINE_APPLICATION_H

#include "Hazel/Core.h"

#include "Window.h"
#include "Hazel/LayerStack.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"

#include "Hazel/ImGui/ImGuiLayer.h"

namespace Hazel {
    class HAZEL_API Application {
    public:
        Application();
        virtual ~Application();

        void Run();
        void OnEvent(Event &e);

        void PushLayer(Layer* layer);
        void PushOverLay(Layer* overlay);

        inline Window& GetWindow() { return * m_Window; }

        inline static Application& Get(){ return *s_Instance; }
    private:
        bool OnWindowClose(WindowCloseEvent &e);

        std::unique_ptr<Window> m_Window;
        ImGuiLayer* m_ImGuiLayer;
        bool m_Running = true;
        LayerStack m_LayerStack;

    private:
        static Application* s_Instance;
    };

    // To be defined in CLIENT
    Application *CreateApplication();
}

#endif //HAZELENGINE_APPLICATION_H
