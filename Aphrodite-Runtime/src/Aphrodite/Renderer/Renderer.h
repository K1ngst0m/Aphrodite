//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_RENDERER_H
#define Aphrodite_ENGINE_RENDERER_H

#include "Aphrodite/Renderer/OrthographicCamera.h"
#include "Aphrodite/Renderer/RenderCommand.h"
#include "Aphrodite/Renderer/Shader.h"

namespace Aph {
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
}// namespace Aph

#endif// Aphrodite_ENGINE_RENDERER_H
