//
// Created by npchitman on 6/29/21.
//

#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

#include "pch.h"

namespace Aph {
    OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding) {
        glCreateBuffers(1, &m_RendererID);
        glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
    }

    OpenGLUniformBuffer::~OpenGLUniformBuffer() {
        glDeleteBuffers(1, &m_RendererID);
    }

    void OpenGLUniformBuffer::SetData(const void *data, uint32_t size, uint32_t offset) {
        glNamedBufferSubData(m_RendererID, offset, size, data);
    }

}// namespace Aph-Runtime
