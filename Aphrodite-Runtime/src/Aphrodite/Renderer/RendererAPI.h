//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_RENDERERAPI_H
#define Aphrodite_ENGINE_RENDERERAPI_H

#include <glm/glm.hpp>

#include "Aphrodite/Renderer/VertexArray.h"

namespace Aph {
    class RendererAPI {
    public:
        enum class API { None = 0,
                         OpenGL = 1
                         // TODO: directx
        };

    public:
        virtual ~RendererAPI() = default;

        virtual void Init() = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetClearColor(const glm::vec4 &color) = 0;
        virtual void Clear() = 0;
        virtual void DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t indexCount) = 0;
        virtual void DrawArray(uint32_t first, uint32_t count) = 0;
        virtual void SetDepthMask(bool flag) = 0;
        virtual void SetDepthTest(bool flag) = 0;
        static API GetAPI() { return s_API; }

        static Scope<RendererAPI> Create();
    private:
        static API s_API;
    };
}// namespace Aph-Runtime

#endif// Aphrodite_ENGINE_RENDERERAPI_H
