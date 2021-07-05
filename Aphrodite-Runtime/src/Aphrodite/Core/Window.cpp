//
// Created by npchitman on 6/23/21.
//

#include "Window.h"

#include "pch.h"

#ifdef APH_PLATFORM_LINUX
    #include "Platform/Linux/LinuxWindow.h"
#endif

namespace Aph {

    Scope<Window> Window::Create(const WindowProps& props)
    {
#ifdef APH_PLATFORM_LINUX
        return CreateScope<LinuxWindow>(props);
#else
        APH_CORE_ASSERT(false, "Unknown platform!");
        return nullptr;
#endif
    }

}