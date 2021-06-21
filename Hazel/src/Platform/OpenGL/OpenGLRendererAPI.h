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
        void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        void SetClearColor(const glm::vec4 &color) override;
        void Clear() override;

        void DrawIndexed(const std::shared_ptr<VertexArray> &vertexArray) override;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_OPENGLRENDERERAPI_H
