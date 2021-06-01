//
// Created by npchitman on 6/1/21.
//

#include "hzpch.h"
#include "OpenGLBuffer.h"

#include <glad/glad.h>

Hazel::OpenGLBuffer::OpenGLBuffer(float *vertices, uint32_t size) {
    glCreateBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

Hazel::OpenGLBuffer::~OpenGLBuffer() {
    glDeleteBuffers(1, &m_RendererID);
}

void Hazel::OpenGLBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void Hazel::OpenGLBuffer::UnBind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Hazel::OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t *indices, uint32_t count)
: m_Count(count)
{
    glCreateBuffers(1, &m_RendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}


Hazel::OpenGLIndexBuffer::~OpenGLIndexBuffer() {
    glDeleteBuffers(1, &m_RendererID);
}

void Hazel::OpenGLIndexBuffer::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void Hazel::OpenGLIndexBuffer::UnBind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

