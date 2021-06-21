//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_RENDERCOMMAND_H
#define HAZEL_ENGINE_RENDERCOMMAND_H

#include "RendererAPI.h"

namespace Hazel {
    class RenderCommand {
    public:
        inline static void Init() {
            s_RendererAPI->Init();
        }

        inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
            s_RendererAPI->SetViewport(x, y, width, height);
        }

        inline static void SetClearColor(const glm::vec4 &color) {
            s_RendererAPI->SetClearColor(color);
        }

        inline static void Clear() {
            s_RendererAPI->Clear();
        }

        inline static void
        DrawIndexed(const std::shared_ptr<VertexArray> &vertexArray) {
            s_RendererAPI->DrawIndexed(vertexArray);
        }

    private:
        static RendererAPI *s_RendererAPI;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_RENDERCOMMAND_H
