// Base.h

// marco definition:
// - platform specification
// -
// - encapsulate smart pointer, callback function binding, debug(e.g., assert), etc.

#ifndef Aphrodite_CORE_H
#define Aphrodite_CORE_H

#include <memory>

#include "Aphrodite/Core/PlatformDetection.h"
#include <csignal>
#include <glm/glm.hpp>
#include <string_view>

#ifdef APH_DEBUG
#if defined(APH_PLATFORM_WINDOWS)
#define APH_DEBUGBREAK() __debugbreak()
#elif defined(APH_PLATFORM_LINUX)
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
    namespace Style{
        namespace Title{
            const std::string_view SceneHierarchy = "\uF5FD  Scene Hierarchy";
            const std::string_view Properties = "\uF1B2  Properties";
            const std::string_view Viewport = "\uF06E  Viewport";
            const std::string_view Project = "\uF07B  Project";
            const std::string_view Console = "\uF069  Console";
            const std::string_view RenderInfo = "\uF05A  Render Info";
            const std::string_view Renderer2DStatistics = "\uF05A  Renderer2D Stats";
        }

        namespace Color{
            const glm::vec4 ClearColor = {0.049f, 0.085f, 0.104f, 1.0f};
        }
    }

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