//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_VERTEXARRAY_H
#define Aphrodite_ENGINE_VERTEXARRAY_H

#include <memory>

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Renderer/Buffer.h"

namespace Aph {
    class VertexArray {
    public:
        virtual ~VertexArray() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        virtual void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) = 0;
        virtual void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) = 0;

        virtual const std::vector<Ref<VertexBuffer>> &GetVertexBuffers() const = 0;
        virtual const Ref<IndexBuffer> &GetIndexBuffer() const = 0;

        static Ref<VertexArray> Create();
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_VERTEXARRAY_H
