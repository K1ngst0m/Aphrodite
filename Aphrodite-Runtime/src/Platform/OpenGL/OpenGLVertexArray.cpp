//
// Created by npchitman on 6/1/21.
//

#include "OpenGLVertexArray.h"

#include <glad/glad.h>

#include "pch.h"

namespace Aph {
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
        switch (type) {
            case Aph::ShaderDataType::Float:
                return GL_FLOAT;
            case Aph::ShaderDataType::Float2:
                return GL_FLOAT;
            case Aph::ShaderDataType::Float3:
                return GL_FLOAT;
            case Aph::ShaderDataType::Float4:
                return GL_FLOAT;
            case Aph::ShaderDataType::Mat3:
                return GL_FLOAT;
            case Aph::ShaderDataType::Mat4:
                return GL_FLOAT;
            case Aph::ShaderDataType::Int:
                return GL_INT;
            case Aph::ShaderDataType::Int2:
                return GL_INT;
            case Aph::ShaderDataType::Int3:
                return GL_INT;
            case Aph::ShaderDataType::Int4:
                return GL_INT;
            case Aph::ShaderDataType::Bool:
                return GL_BOOL;
        }

        APH_CORE_ASSERT(false, "Unknown ShaderDataType!");
        return 0;
    }
}// namespace Aph-Runtime

Aph::OpenGLVertexArray::OpenGLVertexArray() {
    APH_PROFILE_FUNCTION();

    glCreateVertexArrays(1, &m_RendererID);
}

Aph::OpenGLVertexArray::~OpenGLVertexArray() {
    APH_PROFILE_FUNCTION();

    glDeleteVertexArrays(1, &m_RendererID);
}

void Aph::OpenGLVertexArray::Bind() const {
    APH_PROFILE_FUNCTION();

    glBindVertexArray(m_RendererID);
}

void Aph::OpenGLVertexArray::UnBind() const {
    APH_PROFILE_FUNCTION();

    glBindVertexArray(0);
}

void Aph::OpenGLVertexArray::AddVertexBuffer(
        const std::shared_ptr<VertexBuffer> &vertexBuffer) {
    APH_PROFILE_FUNCTION();

    APH_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(),
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
            case ShaderDataType::Float4: {
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
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::Bool: {
                glEnableVertexAttribArray(m_VertexBufferIndex);
                glVertexAttribIPointer(m_VertexBufferIndex,
                                       static_cast<GLint>(element.GetComponentCount()),
                                       ShaderDataTypeToOpenGLBaseType(element.Type),
                                       static_cast<GLsizei>(layout.GetStride()),
                                       (const void *) element.Offset);
                m_VertexBufferIndex++
                        ;
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
                APH_CORE_ASSERT(false, "Unknown ShaderDataType!");
        }
    }
    m_VertexBuffers.push_back(vertexBuffer);
}

void Aph::OpenGLVertexArray::SetIndexBuffer(
        const std::shared_ptr<IndexBuffer> &indexBuffer) {
    APH_PROFILE_FUNCTION();

    glBindVertexArray(m_RendererID);
    indexBuffer->Bind();

    m_IndexBuffer = indexBuffer;
}
