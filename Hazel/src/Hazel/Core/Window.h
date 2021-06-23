//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_WINDOW_H
#define HAZEL_ENGINE_WINDOW_H

#include <utility>

#include "Base.h"
#include "Hazel/Events/Event.h"
#include "hzpch.h"

namespace Hazel {
    struct WindowProps {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        explicit WindowProps(std::string title = "Hazel Engine",
                             unsigned int width = 1280, unsigned int height = 720)
            : Title(std::move(title)),
              Width(width),
              Height(height) {}
    };

    class Window {
    public:
        using EventCallbackFn = std::function<void(Event &)>;

        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual unsigned int GetWidth() const = 0;

        virtual unsigned int GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;

        virtual void SetVSync(bool enabled) = 0;

        virtual bool IsVSync() const = 0;

        virtual void *GetNativeWindow() const = 0;

        static Scope<Window> Create(const WindowProps &props = WindowProps());
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_WINDOW_H
