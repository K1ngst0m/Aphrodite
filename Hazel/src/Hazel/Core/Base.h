// Base.h

// marco definition:
// - platform specification
// -
// - encapsulate smart pointer, callback function binding, debug(e.g., assert), etc.

#ifndef HAZEL_CORE_H
#define HAZEL_CORE_H

#include <memory>

#include "Hazel/Core/PlatformDetection.h"

#ifdef HZ_DEBUG
#if defined(HZ_PLATFORM_WINDOWS)
#define HZ_DEBUGBREAK() __debugbreak()
#elif defined(HZ_PLATFORM_LINUX)
#include <csignal>
#define HZ_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#else
#define HZ_DEBUGBREAK()
#endif

#define HZ_EXPAND_MACRO(x) x
#define HZ_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define HZ_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Hazel {

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
#endif// HAZEL_CORE_H

#include "Hazel/Core/Log.h"
#include "Hazel/Core/Assert.h"