//
// Created by npchitman on 6/1/21.
//

#include "Shader.h"

#include "Platform/OpenGL/OpenGLShader.h"
#include "Renderer.h"
#include "pch.h"

namespace Aph {
    Ref<Shader> Shader::Create(const std::string &filepath) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLShader>(filepath);
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    Ref<Shader> Shader::Create(const std::string &name,
                               const std::string &vertexSrc,
                               const std::string &fragmentSrc) {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                APH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc);
            default:
                APH_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }


    // shader library

    void ShaderLibrary::Add(const std::string &name,
                            const Ref<Shader> &shader) {
        APH_CORE_ASSERT(Exists(name), "Shader already exists!");
        m_Shaders[name] = shader;
    }

    void ShaderLibrary::Add(const Ref<Shader> &shader) {
        auto &name = shader->GetName();
        Add(name, shader);
    }

    Ref<Shader> ShaderLibrary::Load(const std::string &filepath) {
        auto shader = Shader::Create(filepath);
        Add(shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string &name, const std::string &filepath) {
        auto shader = Shader::Create(filepath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string &name) {
        APH_CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Shaders[name];
    }

    bool ShaderLibrary::Exists(const std::string &name) const {
        return m_Shaders.find(name) != m_Shaders.end();
    }
}// namespace Aph
