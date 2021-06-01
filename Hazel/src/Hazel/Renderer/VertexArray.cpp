//
// Created by npchitman on 6/1/21.
//

#include "hzpch.h"
#include "VertexArray.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

Hazel::VertexArray *Hazel::VertexArray::Create() {
    switch (Renderer::GetAPI()) {

        case RendererAPI::API::None:
            HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
        case RendererAPI::API::OpenGL:
            return new OpenGLVertexArray();
        default:
            HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
    }
}
