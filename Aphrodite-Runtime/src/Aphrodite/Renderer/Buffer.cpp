//
// Created by npchitman on 6/1/21.
//

#include "Aphrodite/Renderer/Buffer.h"

#include "Aphrodite/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"
#include "pch.h"

namespace Aph {

    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(size);
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(float *vertices,
                                           uint32_t size) {
        switch (Renderer::GetAPI()) {

            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t *indices,
                                         uint32_t count) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLIndexBuffer>(indices, count);
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    Ref<UniformBuffer> UniformBuffer::Create() {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
            APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLUniformBuffer>();
            default:
            APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}// namespace Aph-Runtime
