//
// Created by npchitman on 6/1/21.
//

#ifndef HAZEL_ENGINE_RENDERER_H
#define HAZEL_ENGINE_RENDERER_H

#include "Hazel/Renderer/OrthographicCamera.h"
#include "Hazel/Renderer/RenderCommand.h"
#include "Hazel/Renderer/Shader.h"

namespace Hazel {
    class Renderer {
    public:
        static void Init();
        static void Shutdown();
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void BeginScene(OrthographicCamera &camera);
        static void EndScene();

        static void Submit(const Ref<Shader> &shader,
                           const Ref<VertexArray> &vertexArray,
                           const glm::mat4 &transform = glm::mat4(1.0f));

        static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
        };

        static Scope<SceneData> s_SceneData;
    };
}// namespace Hazel

#endif// HAZEL_ENGINE_RENDERER_H
