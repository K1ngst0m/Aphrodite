//
// Created by npchitman on 6/1/21.
//

#include "Aphrodite/Renderer/GraphicsContext.h"

#include <Platform/OpenGL/OpenGLContext.h>

#include "Aphrodite/Renderer/Renderer.h"
#include "pch.h"

namespace Aph {

    Scope<GraphicsContext> GraphicsContext::Create(void* window) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
        }

        APH_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}// namespace Aph
