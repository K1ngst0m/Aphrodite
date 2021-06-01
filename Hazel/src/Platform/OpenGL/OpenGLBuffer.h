//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_OPENGLBUFFER_H
#define HAZEL_ENGINE_OPENGLBUFFER_H

#include "Hazel/Renderer/Buffer.h"

namespace Hazel {
    class OpenGLBuffer : public VertexBuffer {
    public:
        OpenGLBuffer(float *vertices, uint32_t size);

        ~OpenGLBuffer() override;

        void Bind() const override;

        void UnBind() const override;

        const BufferLayout &GetLayout() const override { return m_Layout; }

        void SetLayout(const BufferLayout &layout) override { m_Layout = layout; }

    private:
        uint32_t m_RendererID;
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
        uint32_t m_RendererID;
        uint32_t m_Count;
    };
}


#endif //HAZEL_ENGINE_OPENGLBUFFER_H
