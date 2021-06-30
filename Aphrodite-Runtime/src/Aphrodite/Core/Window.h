// Window.h

// WindowProps:

// Window class: abstract class
// - create and delete window
// - get and set some window property (height, width, vsync, etc.)
// - window event callback

#ifndef Aphrodite_ENGINE_WINDOW_H
#define Aphrodite_ENGINE_WINDOW_H

#include <utility>

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Events/Event.h"
#include <sstream>

namespace Aph {
    struct WindowProps {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        explicit WindowProps(std::string title = "Aph-Runtime Engine",
                             uint32_t width = 1600, uint32_t height = 900)
            : Title(std::move(title)),
              Width(width),
              Height(height) {}
    };

    class Window {
    public:
        using EventCallbackFn = std::function<void(Event &)>;

        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;

        virtual uint32_t GetHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;

        virtual void SetVSync(bool enabled) = 0;

        virtual bool IsVSync() const = 0;

        virtual void *GetNativeWindow() const = 0;

        static Scope<Window> Create(const WindowProps &props = WindowProps());
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_WINDOW_H
