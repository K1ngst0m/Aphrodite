//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_OPENGLRENDERERAPI_H
#define HAZEL_ENGINE_OPENGLRENDERERAPI_H

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {
    class OpenGLRendererAPI : public RendererAPI {
    public:
        void Init() override;
        void SetClearColor(const glm::vec4 &color) override;
        void Clear() override;

        void DrawIndexed(const std::shared_ptr<VertexArray> &vertexArray) override;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_OPENGLRENDERERAPI_H
