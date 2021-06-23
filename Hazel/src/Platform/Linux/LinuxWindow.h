//
// Created by npchitman on 5/31/21.
//

#ifndef HAZEL_ENGINE_LINUXWINDOW_H
#define HAZEL_ENGINE_LINUXWINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Hazel/Core/Window.h"
#include "Hazel/Renderer/GraphicsContext.h"

namespace Hazel {
    class LinuxWindow : public Window {
    public:
        explicit LinuxWindow(const WindowProps &props);

        ~LinuxWindow() override;

        void OnUpdate() override;

        unsigned int GetWidth() const override { return m_Data.Width; }

        unsigned int GetHeight() const override { return m_Data.Height; }

        void SetEventCallback(const EventCallbackFn &callback) override {
            m_Data.EventCallback = callback;
        }

        void SetVSync(bool enabled) override;

        bool IsVSync() const override;

        void *GetNativeWindow() const override { return m_Window; }

    private:
        virtual void Init(const WindowProps &props);

        virtual void Shutdown();

    private:
        GLFWwindow *m_Window;
        Scope<GraphicsContext> m_Context;

        struct WindowData {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_LINUXWINDOW_H
