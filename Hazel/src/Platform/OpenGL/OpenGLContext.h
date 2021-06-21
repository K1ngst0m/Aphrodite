//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_OPENGLCONTEXT_H
#define HAZEL_ENGINE_OPENGLCONTEXT_H

#include "Hazel/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Hazel {
    class OpenGLContext : public GraphicsContext {
    public:
        explicit OpenGLContext(GLFWwindow *windowHandle);

        void Init() override;
        void SwapBuffers() override;

    private:
        GLFWwindow *m_WindowHandle;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_OPENGLCONTEXT_H
