//
// Created by npchitman on 6/1/21.
//

#include "hzpch.h"
#include "Buffer.h"

#include "Renderer.h"

#include "Platform/OpenGL/OpenGLBuffer.h"

Hazel::VertexBuffer *Hazel::VertexBuffer::Create(float *vertices, uint32_t size) {
    switch (Renderer::GetAPI()) {

        case Hazel::RendererAPI::None:
            HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case Hazel::RendererAPI::OpenGL:
            return new OpenGLBuffer(vertices, size);
    }

    HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Hazel::IndexBuffer *Hazel::IndexBuffer::Create(uint32_t *indices, uint32_t size) {
    switch (Renderer::GetAPI()) {
        case Hazel::RendererAPI::None:
            HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case Hazel::RendererAPI::OpenGL:
            return new OpenGLIndexBuffer(indices, size);
    }
}
