//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_OPENGLCONTEXT_H
#define HAZEL_ENGINE_OPENGLCONTEXT_H

#include <Hazel/Renderer/Buffer.h>

#include "Hazel/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Hazel {
    class OpenGLContext : public GraphicsContext {
    public:
        explicit OpenGLContext(GLFWwindow *windowHandle);

        void Init() override;
        void SwapBuffers() override;

        static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t size);
    private:
        GLFWwindow *m_WindowHandle;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_OPENGLCONTEXT_H
