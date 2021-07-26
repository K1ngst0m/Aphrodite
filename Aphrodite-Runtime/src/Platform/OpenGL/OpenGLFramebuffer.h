//
// Created by npchitman on 6/24/21.
//

#ifndef Aphrodite_ENGINE_OPENGLFRAMEBUFFER_H
#define Aphrodite_ENGINE_OPENGLFRAMEBUFFER_H

#include "Aphrodite/Renderer/Framebuffer.h"

namespace Aph {
    class OpenGLFramebuffer : public Framebuffer {
    public:
        explicit OpenGLFramebuffer(FramebufferSpecification spec);
        ~OpenGLFramebuffer() override;

        void Invalidate();

        void Bind() override;
        void UnBind() override;

        void Resize(uint32_t width, uint32_t height) override;
        int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
        void ClearAttachment(uint32_t attachmentIndex, int value) override;

        uint32_t GetColorAttachmentRendererID(uint32_t index) const override {
            APH_CORE_ASSERT(index < m_ColorAttachments.size());
            return m_ColorAttachments[index];
        }

        const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    private:
        uint32_t m_RendererID = 0;
        FramebufferSpecification m_Specification;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        FramebufferTextureSpecification m_DepthAttachmentSpecification{FramebufferTextureFormat::None};

        std::vector<uint32_t> m_ColorAttachments;
        uint32_t m_DepthAttachment = 0;
    };

}// namespace Aph


#endif//Aphrodite_ENGINE_OPENGLFRAMEBUFFER_H
