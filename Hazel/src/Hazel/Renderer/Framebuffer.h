//
// Created by npchitman on 6/24/21.
//

#ifndef HAZEL_ENGINE_FRAMEBUFFER_H
#define HAZEL_ENGINE_FRAMEBUFFER_H

#include "Hazel/Core/Base.h"

namespace Hazel {
    enum class FramebufferTextureFormat {
        None = 0,

        // Color
        RGBA8,

        // Depth/stencil
        DEPTH24STENCIL8,

        // Defaults
        Depth = DEPTH24STENCIL8
    };

    struct FramebufferTextureSpecification {
        FramebufferTextureSpecification() = default;
        FramebufferTextureSpecification(FramebufferTextureFormat format) : TextureFormat(format) {} // NOLINT(google-explicit-constructor)

        FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
    };

    struct FramebufferAttachmentSpecification {
        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments) : Attachments(attachments) {}

        std::vector<FramebufferTextureSpecification> Attachments;
    };

    struct FramebufferSpecification {
        uint32_t Width{}, Height{};

        FramebufferAttachmentSpecification Attachments;

        uint32_t Samples = 1;
        bool SwapChainTarget = false;
    };

    class Framebuffer {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void UnBind() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual uint32_t GetColorAttachmentRendererID(uint32_t index) const = 0;
        virtual const FramebufferSpecification& GetSpecification() const = 0;
        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_FRAMEBUFFER_H
