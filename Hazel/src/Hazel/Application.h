//
// Created by Npchitman on 2021/1/17.
//

#ifndef HAZELENGINE_APPLICATION_H
#define HAZELENGINE_APPLICATION_H

#include "Window.h"

#include "Hazel/Core.h"
#include "Hazel/LayerStack.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"

namespace Hazel {
    class HAZEL_API Application {
    public:
        Application();
        virtual ~Application();

        void Run();
        void OnEvent(Event &e);

        void PushLayer(Layer* layer);
        void PushOverLay(Layer* overlay);

    private:
        bool OnWindowClose(WindowCloseEvent &e);

        std::unique_ptr<Window> m_Window;
        bool m_Running = true;
        LayerStack m_LayerStack;
    };

    // To be defined in CLIENT
    Application *CreateApplication();
}

#endif //HAZELENGINE_APPLICATION_H
