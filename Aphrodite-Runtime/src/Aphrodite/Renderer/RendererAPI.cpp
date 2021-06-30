//
// Created by npchitman on 6/1/21.
//

#include "Aphrodite/Renderer/RendererAPI.h"

#include <Platform/OpenGL/OpenGLRendererAPI.h>

#include "pch.h"

namespace Aph {
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

    Scope<RendererAPI> RendererAPI::Create() {
        switch (s_API) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateScope<OpenGLRendererAPI>();
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}// namespace Aph-Runtime
