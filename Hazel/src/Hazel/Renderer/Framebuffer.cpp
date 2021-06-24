//
// Created by npchitman on 6/24/21.
//

#include "hzpch.h"
#include "Hazel/Renderer/Framebuffer.h"

#include "Hazel/Renderer/Renderer.h"

#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace Hazel {
    Ref<Hazel::Framebuffer> Hazel::Framebuffer::Create(const Hazel::FramebufferSpecification& spec) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLFramebuffer>(spec);
            default:
                HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}// namespace Hazel
