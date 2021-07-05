// Base.h

// marco definition:
// - platform specification
// -
// - encapsulate smart pointer, callback function binding, debug(e.g., assert), etc.

#ifndef Aphrodite_CORE_H
#define Aphrodite_CORE_H

#include <csignal>
#include <glm/glm.hpp>
#include <memory>
#include <string_view>

#include "Aphrodite/Utils/PlatformDetection.h"

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

#define BIT(x) (1 << (x))

#define APH_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#include <imgui.h>
namespace Aph {
    namespace Style{

        namespace Title{
            const static std::string_view SceneHierarchy = "\uF5FD Scene Hierarchy";
            const static std::string_view Properties = "\uF1B2 Properties";
            const static std::string_view Viewport = "\uF06E Viewport";
            const static std::string_view Project = "\uF07B Project";
            const static std::string_view Console = "\uF120 Console";
            const static std::string_view RenderInfo = "\uF05A Render Info";
            const static std::string_view Renderer2DStatistics = "\uF05A Renderer2D Stats";
        }

        namespace Color{
            const static glm::vec4 ClearColor = {0.049f, 0.085f, 0.104f, 1.0f};
            const static ImVec4 ForegroundColor_white = {0.8f, 0.8f, 0.8f, 1.0f};
            const static ImVec4 ForegroundColor_primary = {0.406f, 0.738f, 0.687f, 1.0f};
            const static ImVec4 ForegroundColor_second = {0.406f, 0.738f, 0.687f, 1.0f};
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

#include "Aphrodite/Debug/Log.h"
#include "Aphrodite/Debug/Assert.h"
