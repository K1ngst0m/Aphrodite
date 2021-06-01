//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_RENDERER_H
#define HAZEL_ENGINE_RENDERER_H

#include "RenderCommand.h"

#include "OrthographicCamera.h"
#include "Shader.h"

namespace Hazel {
    class Renderer {
    public:
        static void BeginScene(OrthographicCamera& camera);
        static void EndScene();

        static void Submit(const std::shared_ptr<Shader>& shader,
                           const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    private:
        struct SceneData{
            glm::mat4 ViewProjectionMatrix;
        };

        static SceneData* s_SceneData;
    };
}


#endif //HAZEL_ENGINE_RENDERER_H
