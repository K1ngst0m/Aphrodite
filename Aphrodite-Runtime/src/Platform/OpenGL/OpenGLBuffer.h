//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_OPENGLBUFFER_H
#define Aphrodite_ENGINE_OPENGLBUFFER_H

#include "Aphrodite/Renderer/Buffer.h"

namespace Aph {
    class OpenGLVertexBuffer : public VertexBuffer {
    public:
        explicit OpenGLVertexBuffer(uint32_t size);
        OpenGLVertexBuffer(float *vertices, uint32_t size);
        ~OpenGLVertexBuffer() override;

        void Bind() const override;
        void UnBind() const override;

        void SetData(const void *data, uint32_t size) override;

        const BufferLayout &GetLayout() const override { return m_Layout; }
        void SetLayout(const BufferLayout &layout) override { m_Layout = layout; }

    private:
        uint32_t m_RendererID{};
        BufferLayout m_Layout;
    };

    class OpenGLIndexBuffer : public IndexBuffer {
    public:
        OpenGLIndexBuffer(uint32_t *indices, uint32_t count);
        ~OpenGLIndexBuffer() override;

        void Bind() const override;
        void UnBind() const override;

        uint32_t GetCount() const override { return m_Count; }

    private:
        uint32_t m_RendererID{};
        uint32_t m_Count;
    };

    class OpenGLUniformBuffer : public UniformBuffer {
    public:
        OpenGLUniformBuffer();
        ~OpenGLUniformBuffer() override;

        void Bind() const override;
        void Unbind() const override;

        void SetData(void* data, uint32_t offset, uint32_t size) override;
        void SetLayout(const BufferLayout& layout, uint32_t blockIndex, uint32_t count) override;

    private:
        uint32_t m_RendererID{};
        BufferLayout m_Layout;
    };

}// namespace Aph

#endif// Aphrodite_ENGINE_OPENGLBUFFER_H
