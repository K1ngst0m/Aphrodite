//
// Created by npchitman on 6/1/21.
//

#include "VertexArray.h"

#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Renderer.h"
#include "pch.h"

namespace Aph {
    Ref<VertexArray> Aph::VertexArray::Create() {
        switch (Renderer::GetAPI()) {

            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexArray>();
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}

