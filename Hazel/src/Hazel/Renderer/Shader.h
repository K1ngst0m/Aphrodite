//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_SHADER_H
#define HAZEL_ENGINE_SHADER_H

#include <string>
#include <glm/glm.hpp>


namespace Hazel{
    class Shader final{
    public:
        Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~Shader();

        void Bind() const;
        void UnBind() const;

        void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
    private:
        uint32_t m_RendererID;
    };
}


#endif //HAZEL_ENGINE_SHADER_H
