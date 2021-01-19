//
// Created by Npchitman on 2021/1/17.
//

#include "hzpch.h"
#include "Application.h"

#include "ApplicationEvent.h"
#include "Log.h"

namespace Hazel{
    Application::Application()= default;
    Application::~Application()= default;

    [[noreturn]] void Application::Run() {
        WindowResizeEvent e(1280, 720);
        if(e.IsInCategory(EventCategoryApplication)){
            HZ_TRACE(e);
        }
        if(e.IsInCategory(EventCategoryInput)){
            HZ_TRACE(e);
        }
        while(true);
    }
}
