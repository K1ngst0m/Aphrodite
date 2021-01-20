//
// Created by Npchitman on 2021/1/17.
//

#ifndef HAZELENGINE_APPLICATION_H
#define HAZELENGINE_APPLICATION_H

#include "hzpch.h"
#include "Core.h"
#include "Events/Event.h"
#include "Window.h"

namespace Hazel{
    class HAZEL_API Application{
    public:
        Application();
        virtual ~Application();

        void Run() const;

        void OnEvent(Event &e);
    private:
        std::unique_ptr<Window> m_Window;
        bool m_Running = true;
    };

    // To be defined in CLIENT
    Application* CreateApplication();
}

#endif //HAZELENGINE_APPLICATION_H
