//
// Created by npchitman on 6/23/21.
//

#include "hzpch.h"
#include "Hazel/Core/Window.h"

#ifdef HZ_PLATFORM_LINUX
    #include "Platform/Linux/LinuxWindow.h"
#endif

namespace Hazel
{

    Scope<Window> Window::Create(const WindowProps& props)
    {
#ifdef HZ_PLATFORM_LINUX
        return CreateScope<LinuxWindow>(props);
#else
        HZ_CORE_ASSERT(false, "Unknown platform!");
        return nullptr;
#endif
    }

}