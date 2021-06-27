//
// Created by npchitman on 6/1/21.
//

#include "OpenGLVertexArray.h"

#include <glad/glad.h>

#include "hzpch.h"

namespace Hazel {
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
        switch (type) {
            case Hazel::ShaderDataType::Float:
                return GL_FLOAT;
            case Hazel::ShaderDataType::Float2:
                return GL_FLOAT;
            case Hazel::ShaderDataType::Float3:
                return GL_FLOAT;
            case Hazel::ShaderDataType::Float4:
                return GL_FLOAT;
            case Hazel::ShaderDataType::Mat3:
                return GL_FLOAT;
            case Hazel::ShaderDataType::Mat4:
                return GL_FLOAT;
            case Hazel::ShaderDataType::Int:
                return GL_INT;
            case Hazel::ShaderDataType::Int2:
                return GL_INT;
            case Hazel::ShaderDataType::Int3:
                return GL_INT;
            case Hazel::ShaderDataType::Int4:
                return GL_INT;
            case Hazel::ShaderDataType::Bool:
                return GL_BOOL;
        }

        HZ_CORE_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }
}// namespace Hazel

Hazel::OpenGLVertexArray::OpenGLVertexArray() {
    HZ_PROFILE_FUNCTION();

    glCreateVertexArrays(1, &m_RendererID);
}

Hazel::OpenGLVertexArray::~OpenGLVertexArray() {
    HZ_PROFILE_FUNCTION();

    glDeleteVertexArrays(1, &m_RendererID);
}

void Hazel::OpenGLVertexArray::Bind() const {
    HZ_PROFILE_FUNCTION();

    glBindVertexArray(m_RendererID);
}

void Hazel::OpenGLVertexArray::UnBind() const {
    HZ_PROFILE_FUNCTION();

    glBindVertexArray(0);
}

void Hazel::OpenGLVertexArray::AddVertexBuffer(
        const std::shared_ptr<VertexBuffer> &vertexBuffer) {
    HZ_PROFILE_FUNCTION();

    HZ_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(),
                   "Vertex Buffer has no layout!");
    glBindVertexArray(m_RendererID);
    vertexBuffer->Bind();

    uint32_t index = 0;

    const auto &layout = vertexBuffer->GetLayout();

    for (const auto &element : layout) {
        switch (element.Type) {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::Bool: {
                glEnableVertexAttribArray(m_VertexBufferIndex);
                glVertexAttribPointer(m_VertexBufferIndex,
                                      static_cast<GLint>(element.GetComponentCount()),
                                      ShaderDataTypeToOpenGLBaseType(element.Type),
                                      element.Normalized ? GL_TRUE : GL_FALSE,
                                      static_cast<GLsizei>(layout.GetStride()),
                                      (const void *) element.Offset);
                m_VertexBufferIndex++;
                break;
            }
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4: {
                uint8_t count = element.GetComponentCount();
                for (uint8_t i = 0; i < count; i++) {
                    glEnableVertexAttribArray(m_VertexBufferIndex);
                    glVertexAttribPointer(m_VertexBufferIndex,
                                          count,
                                          ShaderDataTypeToOpenGLBaseType(element.Type),
                                          element.Normalized ? GL_TRUE : GL_FALSE,
                                          static_cast<GLsizei>(layout.GetStride()),
                                          (const void *) (element.Offset + sizeof(float) * count * i));
                    glVertexAttribDivisor(m_VertexBufferIndex, 1);
                    m_VertexBufferIndex++;
                }
                break;
            }
            default:
                HZ_CORE_ASSERT(false, "Unknown ShaderDataType!");
        }
    }
    m_VertexBuffers.push_back(vertexBuffer);
}

void Hazel::OpenGLVertexArray::SetIndexBuffer(
        const std::shared_ptr<IndexBuffer> &indexBuffer) {
    HZ_PROFILE_FUNCTION();

    glBindVertexArray(m_RendererID);
    indexBuffer->Bind();

    m_IndexBuffer = indexBuffer;
}
