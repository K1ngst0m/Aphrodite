//
// Created by npchitman on 6/23/21.
//

#include "Hazel/Core/Input.h"

#include "hzpch.h"

#ifdef HZ_PLATFORM_LINUX
#include "Platform/Linux/LinuxInput.h"
#endif

namespace Hazel {
    Scope<Input> Input::s_Instance = Input::Create();

    Scope<Input> Input::Create() {
#ifdef HZ_PLATFORM_LINUX
        return CreateScope<LinuxInput>();
#else
        HZ_CORE_ASSERT(false, "Unknown platform");
        return nullptr;
#endif
    }


}// namespace Hazel
