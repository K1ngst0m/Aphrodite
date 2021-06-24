//
// Created by npchitman on 6/24/21.
//

#ifndef HAZEL_ENGINE_FRAMEBUFFER_H
#define HAZEL_ENGINE_FRAMEBUFFER_H

#include "Hazel/Core/Base.h"

namespace Hazel {
    struct FramebufferSpecification {
        uint32_t Width{}, Height{};

        uint32_t Samples = 1;
        bool SwapChainTarget = false;
    };

    class Framebuffer{
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void UnBind() = 0;

        virtual uint32_t GetColorAttachmentRendererID() const = 0;
        virtual const FramebufferSpecification& GetSpecification() const = 0;
        static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
    };
}// namespace Hazel


#endif//HAZEL_ENGINE_FRAMEBUFFER_H
