//
// Created by npchitman on 6/20/21.
//

#include "OpenGLShader.h"

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "hzpch.h"

namespace Hazel {

    OpenGLShader::OpenGLShader(const std::string &vertexSrc,
                               const std::string &fragmentSrc) {
        // create an empty vertex shader handle
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

        // Send the vertex Shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        const GLchar *source = vertexSrc.c_str();
        glShaderSource(vertexShader, 1, &source, nullptr);

        // Compile the vertex shader
        glCompileShader(vertexShader);

        GLint isCompiled = 0;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(vertexShader, maxLength, &maxLength, infoLog.data());

            glDeleteShader(vertexShader);

            HZ_CORE_ERROR("{0}", infoLog.data());
            HZ_CORE_ASSERT(false, "vertex shader compilation failure");
            return;
        }

        // create an empty fragment shader handle
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        // Send the fragment shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        source = fragmentSrc.c_str();
        glShaderSource(fragmentShader, 1, &source, nullptr);

        // Compile the fragment shader
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, infoLog.data());

            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            HZ_CORE_ERROR("{0}", infoLog.data());
            HZ_CORE_ASSERT(false, "Fragment shader compilation failure!");
            return;
        }

        m_RendererID = glCreateProgram();
        GLuint program = m_RendererID;

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int *) &isLinked);
        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

            glDeleteProgram(program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            HZ_CORE_ERROR("{0}", infoLog.data());
            HZ_CORE_ASSERT(false, "Shader link failure!");
            return;
        }

        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);
    }

    OpenGLShader::~OpenGLShader() { glDeleteProgram(m_RendererID); }

    void OpenGLShader::Bind() const { glUseProgram(m_RendererID); }

    void OpenGLShader::UnBind() const { glUseProgram(0); }

    void OpenGLShader::UploadUniformInt(const std::string &name, int value) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, value);
    }

    void OpenGLShader::UploadUniformFloat(const std::string &name, float value) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1f(location, value);
    }

    void OpenGLShader::UploadUniformFloat2(const std::string &name,
                                           const glm::vec2 &value) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform2f(location, value.x, value.y);
    }

    void OpenGLShader::UploadUniformFloat3(const std::string &name,
                                           const glm::vec3 &value) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform3f(location, value.x, value.y, value.z);
    }

    void OpenGLShader::UploadUniformFloat4(const std::string &name,
                                           const glm::vec4 &value) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::UploadUniformMat3(const std::string &name,
                                         const glm::mat3 &matrix) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void OpenGLShader::UploadUniformMat4(const std::string &name,
                                         const glm::mat4 &matrix) const {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
}// namespace Hazel
