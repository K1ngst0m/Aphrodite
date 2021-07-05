//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_OPENGLVERTEXARRAY_H
#define Aphrodite_ENGINE_OPENGLVERTEXARRAY_H

#include "Aphrodite/Renderer/VertexArray.h"

namespace Aph {
    class OpenGLVertexArray : public VertexArray {
    public:
        OpenGLVertexArray();
        ~OpenGLVertexArray() override;

        void Bind() const override;
        void UnBind() const override;

        void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) override;
        void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) override;

        const std::vector<Ref<VertexBuffer>> &GetVertexBuffers() const override { return m_VertexBuffers; }
        const Ref<IndexBuffer> &GetIndexBuffer() const override { return m_IndexBuffer; }

    private:
        uint32_t m_RendererID{};
        uint32_t m_VertexBufferIndex = 0;
        std::vector<Ref<VertexBuffer>> m_VertexBuffers;
        Ref<IndexBuffer> m_IndexBuffer;
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_OPENGLVERTEXARRAY_H
