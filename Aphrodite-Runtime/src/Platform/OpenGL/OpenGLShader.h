//
// Created by npchitman on 6/20/21.
//

#ifndef Aphrodite_ENGINE_OPENGLSHADER_H
#define Aphrodite_ENGINE_OPENGLSHADER_H

#include <glm/glm.hpp>

#include "Aphrodite/Renderer/Shader.h"

typedef unsigned int GLenum;

namespace Aph {
    class OpenGLShader : public Shader {
    public:
        explicit OpenGLShader(const std::string &filepath);
        OpenGLShader(std::string name, const std::string &vertexSrc, const std::string &fragmentSrc);
        ~OpenGLShader() override;

        void Bind() const override;
        void UnBind() const override;

        void SetBool(const std::string& name, bool value) override;
        void SetInt(const std::string& name, int value) override;
        void SetIntArray(const std::string& name, int* value, uint32_t count) override;
        void SetFloat(const std::string& name, float value) override;
        void SetFloat2(const std::string& name, const glm::vec2& value) override;
        void SetFloat3(const std::string& name, const glm::vec3& value) override;
        void SetFloat4(const std::string& name, const glm::vec4& value) override;
        void SetMat4(const std::string& name, const glm::mat4& value) override;

        void SetUniformBlock(const std::string& name, uint32_t blockIndex) override;

        const std::string &GetName() const override { return m_Name; }

        void UploadUniformInt(const std::string &name, int value) const;
        void UploadUniformIntArray(const std::string& name, int* values, uint32_t count) const;

        void UploadUniformFloat(const std::string &name, float value) const;
        void UploadUniformFloat2(const std::string &name, const glm::vec2 &value) const;
        void UploadUniformFloat3(const std::string &name, const glm::vec3 &value) const;
        void UploadUniformFloat4(const std::string &name, const glm::vec4 &value) const;

        void UploadUniformMat3(const std::string &name, const glm::mat3 &matrix) const;
        void UploadUniformMat4(const std::string &name, const glm::mat4 &matrix) const;

    private:
        static std::string ReadFile(const std::string &filepath);
        static std::unordered_map<GLenum, std::string> PreProcess(const std::string &source);
        void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);

#if vulkan_not_yet
        void CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources);
        void CompileOrGetOpenGLBinaries();
        void CreateProgram();
        void Reflect(GLenum stage, const std::vector<uint32_t>& shaderData);
#endif

    private:
        uint32_t m_RendererID{};
        std::string m_FilePath;
        std::string m_Name;

        std::unordered_map<GLenum, std::vector<uint32_t>> m_VulkanSPIRV;
        std::unordered_map<GLenum, std::vector<uint32_t>> m_OpenGLSPIRV;

        std::unordered_map<GLenum, std::string> m_OpenGLSourceCode;
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_OPENGLSHADER_H
