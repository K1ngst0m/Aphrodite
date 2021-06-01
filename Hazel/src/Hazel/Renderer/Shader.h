//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_SHADER_H
#define HAZEL_ENGINE_SHADER_H

#include <string>


namespace Hazel{
    class Shader final{
    public:
        Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~Shader();

        void Bind() const;
        void UnBind() const;

    private:
        uint32_t m_RendererID;
    };
}


#endif //HAZEL_ENGINE_SHADER_H
