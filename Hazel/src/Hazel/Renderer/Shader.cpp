//
// Created by npchitman on 6/1/21.
//

#include "Shader.h"

#include "Platform/OpenGL/OpenGLShader.h"
#include "Renderer.h"
#include "hzpch.h"

namespace Hazel {
    Shader *Shader::Create(const std::string &filepath) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLShader(filepath);
            default:
                HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    Shader *Shader::Create(const std::string &vertexSrc,
                           const std::string &fragmentSrc) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLShader(vertexSrc, fragmentSrc);
            default:
                HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}// namespace Hazel
