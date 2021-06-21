//
// Created by npchitman on 6/21/21.
//

#include "Texture.h"

#include "Platform/OpenGL/OpenGLTexture.h"
#include "Renderer.h"
#include "hzpch.h"

namespace Hazel {

    Ref<Texture2D> Texture2D::Create(const std::string& path) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLTexture2D>(path);
        }

        HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}// namespace Hazel