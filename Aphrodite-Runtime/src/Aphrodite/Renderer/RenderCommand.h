//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_RENDERCOMMAND_H
#define Aphrodite_ENGINE_RENDERCOMMAND_H

#include "Aphrodite/Renderer/RendererAPI.h"

namespace Aph {
    class RenderCommand {
    public:
        static void Init() {
            s_RendererAPI->Init();
        }

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
            s_RendererAPI->SetViewport(x, y, width, height);
        }

        static void SetClearColor(const glm::vec4 &color) {
            s_RendererAPI->SetClearColor(color);
        }

        static void Clear() {
            s_RendererAPI->Clear();
        }

        static void
        DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t count = 0) {
            s_RendererAPI->DrawIndexed(vertexArray, count);
        }


        inline static void DrawArray(uint32_t first, uint32_t count)
        {
            s_RendererAPI->DrawArray(first, count);
        }

        inline static void SetDepthMask(bool flag)
        {
            s_RendererAPI->SetDepthMask(flag);
        }

        inline static void SetDepthTest(bool flag)
        {
            s_RendererAPI->SetDepthTest(flag);
        }

    private:
        static Scope<RendererAPI> s_RendererAPI;
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_RENDERCOMMAND_H
