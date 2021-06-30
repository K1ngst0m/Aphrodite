//
// Created by npchitman on 6/24/21.
//

#include "Aphrodite/Renderer/Framebuffer.h"

#include "Aphrodite/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "pch.h"

namespace Aph {
    Ref<Aph::Framebuffer> Aph::Framebuffer::Create(const Aph::FramebufferSpecification& spec) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLFramebuffer>(spec);
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}// namespace Aph-Runtime
