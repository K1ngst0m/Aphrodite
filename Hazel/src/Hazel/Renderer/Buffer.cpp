//
// Created by npchitman on 6/1/21.
//

#include "Hazel/Renderer/Buffer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Hazel/Renderer/Renderer.h"
#include "hzpch.h"

namespace Hazel{
    Ref<VertexBuffer> VertexBuffer::Create(float *vertices,
                                                     uint32_t size) {
        switch (Renderer::GetAPI()) {

            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
        }

        HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t *indices,
                                                   uint32_t size) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLIndexBuffer>(indices, size);
            default:
                HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}
