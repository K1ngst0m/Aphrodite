//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_VERTEXARRAY_H
#define Aphrodite_ENGINE_VERTEXARRAY_H

#include <memory>

#include "Aphrodite/Renderer/Buffer.h"

namespace Aph {
    class VertexArray {
    public:
        virtual ~VertexArray() = default;

        virtual void Bind() const = 0;

        virtual void UnBind() const = 0;

        virtual void
        AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vertexBuffer) = 0;

        virtual void
        SetIndexBuffer(const std::shared_ptr<IndexBuffer> &indexBuffer) = 0;

        virtual const std::vector<std::shared_ptr<VertexBuffer>> &
        GetVertexBuffers() const = 0;

        virtual const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const = 0;

        static Ref<VertexArray> Create();
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_VERTEXARRAY_H
