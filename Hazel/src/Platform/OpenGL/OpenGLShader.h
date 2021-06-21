//
// Created by npchitman on 6/20/21.
//

#ifndef HAZEL_ENGINE_OPENGLSHADER_H
#define HAZEL_ENGINE_OPENGLSHADER_H

#include <glm/glm.hpp>

#include "Hazel/Renderer/Shader.h"

namespace Hazel {
    class OpenGLShader : public Shader {
    public:
        OpenGLShader(const std::string &vertexSrc, const std::string &fragmentSrc);
        ~OpenGLShader() override;

        void Bind() const override;
        void UnBind() const override;

        void UploadUniformInt(const std::string &name, int value) const;
        void UploadUniformFloat(const std::string &name, float value) const;
        void UploadUniformFloat2(const std::string &name, const glm::vec2 &value) const;
        void UploadUniformFloat3(const std::string &name, const glm::vec3 &value) const;
        void UploadUniformFloat4(const std::string &name, const glm::vec4 &value) const;

        void UploadUniformMat3(const std::string &name, const glm::mat3 &matrix) const;
        void UploadUniformMat4(const std::string &name, const glm::mat4 &matrix) const;

    private:
        uint32_t m_RendererID;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_OPENGLSHADER_H
