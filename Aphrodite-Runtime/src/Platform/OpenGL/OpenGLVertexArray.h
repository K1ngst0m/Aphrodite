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

        void
        AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vertexBuffer) override;

        void SetIndexBuffer(const std::shared_ptr<IndexBuffer> &indexBuffer) override;

        const std::vector<std::shared_ptr<VertexBuffer>> &
        GetVertexBuffers() const override {
            return m_VertexBuffers;
        }

        const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const override {
            return m_IndexBuffer;
        }

    private:
        uint32_t m_RendererID{};
        uint32_t m_VertexBufferIndex = 0;
        std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
        std::shared_ptr<IndexBuffer> m_IndexBuffer;
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_OPENGLVERTEXARRAY_H
