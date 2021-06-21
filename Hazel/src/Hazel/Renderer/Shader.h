//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_SHADER_H
#define HAZEL_ENGINE_SHADER_H

#include <string>

namespace Hazel {
    class Shader {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

    private:
        uint32_t m_RendererID{};

    public:
        static Shader *Create(const std::string &vertexSrc,
                              const std::string &fragmentSrc);
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_SHADER_H
