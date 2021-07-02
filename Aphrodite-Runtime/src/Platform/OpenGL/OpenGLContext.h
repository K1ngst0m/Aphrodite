//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_OPENGLCONTEXT_H
#define Aphrodite_ENGINE_OPENGLCONTEXT_H

#include <Aphrodite/Renderer/Buffer.h>

#include "Aphrodite/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Aph {
    class OpenGLContext : public GraphicsContext {
    public:
        explicit OpenGLContext(GLFWwindow *windowHandle);

        void Init() override;
        void SwapBuffers() override;

        ContextInfo GetContextInfo() override { return m_ContextInfo; }

    private:
        GLFWwindow *m_WindowHandle;
        ContextInfo m_ContextInfo{};
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_OPENGLCONTEXT_H
