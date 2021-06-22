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
                return CreateRef<OpenGLTexture2D>(path);
        }

        HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLTexture2D>(width, height);
            default:
                HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}// namespace Hazel