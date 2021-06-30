//
// Created by npchitman on 6/29/21.
//

#ifndef Aphrodite_ENGINE_OPENGLUNIFORMBUFFER_H
#define Aphrodite_ENGINE_OPENGLUNIFORMBUFFER_H


#include "Aphrodite/Renderer/UniformBuffer.h"

namespace Aph {
    class OpenGLUniformBuffer : public UniformBuffer {
    public:
        OpenGLUniformBuffer(uint32_t size, uint32_t binding);
        ~OpenGLUniformBuffer() override;

        void SetData(const void* data, uint32_t size, uint32_t offset /*= 0*/) override;

    private:
        uint32_t m_RendererID = 0;
    };
}// namespace Aph-Runtime


#endif//Aphrodite_ENGINE_OPENGLUNIFORMBUFFER_H
