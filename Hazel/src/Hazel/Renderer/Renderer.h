//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_RENDERER_H
#define HAZEL_ENGINE_RENDERER_H

#include "OrthographicCamera.h"
#include "RenderCommand.h"
#include "Shader.h"

namespace Hazel {
    class Renderer {
    public:
        static void Init();
        static void BeginScene(OrthographicCamera &camera);
        static void EndScene();

        static void Submit(const std::shared_ptr<Shader> &shader,
                           const std::shared_ptr<VertexArray> &vertexArray,
                           const glm::mat4 &transform = glm::mat4(1.0f));

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
        };

        static SceneData *s_SceneData;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_RENDERER_H
