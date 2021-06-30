// Base.h

// marco definition:
// - platform specification
// -
// - encapsulate smart pointer, callback function binding, debug(e.g., assert), etc.

#ifndef Aphrodite_CORE_H
#define Aphrodite_CORE_H

#include <memory>

#include "Aphrodite/Core/PlatformDetection.h"

#ifdef APH_DEBUG
#if defined(APH_PLATFORM_WINDOWS)
#define APH_DEBUGBREAK() __debugbreak()
#elif defined(APH_PLATFORM_LINUX)
#include <csignal>
#define APH_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#else
#define APH_DEBUGBREAK()
#endif

#define APH_EXPAND_MACRO(x) x
#define APH_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define APH_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Aph {

    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

}
#endif// Aphrodite_CORE_H

#include "Aphrodite/Core/Log.h"
#include "Aphrodite/Core/Assert.h"