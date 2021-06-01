//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_RENDERER_H
#define HAZEL_ENGINE_RENDERER_H

#include "RenderCommand.h"

namespace Hazel {
    class Renderer {
    public:
        static void BeginScene();
        static void EndScene();

        static void Submit(const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
    };
}


#endif //HAZEL_ENGINE_RENDERER_H
