//
// Created by npchitman on 6/24/21.
//

#ifndef HAZEL_ENGINE_OPENGLFRAMEBUFFER_H
#define HAZEL_ENGINE_OPENGLFRAMEBUFFER_H

#include "Hazel/Renderer/Framebuffer.h"

namespace Hazel{
    class OpenGLFramebuffer : public Framebuffer{
    public:
        explicit OpenGLFramebuffer(const FramebufferSpecification& spec);
        ~OpenGLFramebuffer() override;

        void Invalidate();

        void Bind() override;
        void UnBind() override;

        void Resize(uint32_t width, uint32_t height) override;

        uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
        const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0, m_DepthAttachment = 0;
        FramebufferSpecification m_Specification;
    };

}


#endif//HAZEL_ENGINE_OPENGLFRAMEBUFFER_H
