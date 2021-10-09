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

#define APH_BIND_EVENT_FN(fn) \
    [this](auto&&... args) -> decltype(auto) \
    { return this->fn(std::forward<decltype(args)>(args)...); }

#include <imgui.h>
#include <unordered_map>

namespace Aph {

    namespace Style {
        namespace FontSize {
            const static float text = 24.0f;
            const static float icon = 17.0f;
        }// namespace FontSize

        namespace Title {
            const static std::string_view SceneHierarchy = "\uF5FD Scene Hierarchy";
            const static std::string_view Properties = "\uF1B2 Properties";
            const static std::string_view Viewport = "\uF06E Scene";
            const static std::string_view Project = "\uF07B Project";
            const static std::string_view Console = "\uF120 Console";
            const static std::string_view RenderInfo = "\uF05A Render Info";
            const static std::string_view Renderer2DStatistics = "\uF05A 2D Render Stats";
            const static std::string_view Settings = "\uF0AD Settings";
        }// namespace Title

        namespace Color {
            // color style
            const auto foreground_1 = ImVec4{0.8f, 0.6f, 0.53f, 1.0f};
            const auto foreground_2 = ImVec4{0.406f, 0.738f, 0.687f, 1.0f};

            const auto background_1 = ImVec4{0.079f, 0.115f, 0.134f, 1.0f};
            const auto background_2 = ImVec4{0.406f, 0.738f, 0.687f, 1.0f};

            const auto background_hovered = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
            const auto background_active = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

            const glm::vec4 Clear = {0.049f, 0.085f, 0.104f, 1.0f};

            struct Vec3Color {
                ImVec4 X;
                ImVec4 Y;
                ImVec4 Z;
            };

            const std::unordered_map<const char*, ImVec4> Background = {
                    {"Primary", {0.406f, 0.738f, 0.687f, 1.0f}},
                    {"Viewport", {0.406f, 0.738f, 0.687f, 1.0f}}};

            const std::unordered_map<const char*, ImVec4> Foreground = {
                    {"White", {0.8f, 0.8f, 0.8f, 1.0f}},
                    {"Primary", {0.406f, 0.738f, 0.687f, 1.0f}},
                    {"Second", {0.406f, 0.738f, 0.687f, 1.0f}}};

            const std::unordered_map<const char*, Vec3Color> Vec3ButtonStyle = {
                    {"Default", {{0.3f, 0.1f, 0.15f, 1.0f}, {0.2f, 0.3f, 0.2f, 1.0f}, {0.1f, 0.25f, 0.4f, 1.0f}}},
                    {"Hovered", {{0.4f, 0.2f, 0.2f, 1.0f}, {0.3f, 0.4f, 0.3f, 1.0f}, {0.2f, 0.35f, 0.5f, 1.0f}}},
                    {"Active", {{0.3f, 0.1f, 0.15f, 1.0f}, {0.2f, 0.3f, 0.2f, 1.0f}, {0.1f, 0.25f, 0.4f, 1.0f}}}};


        }// namespace Color
    }    // namespace Style

    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

}// namespace Aph

#endif// Aphrodite_CORE_H

#include "Aphrodite/Debug/Assert.h"
#include "Aphrodite/Debug/Log.h"
