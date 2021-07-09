//
// Created by npchitman on 6/1/21.
//

#ifndef Aphrodite_ENGINE_RENDERER_H
#define Aphrodite_ENGINE_RENDERER_H

#include "Aphrodite/Renderer/RenderCommand.h"

namespace Aph {
    class EditorCamera;
    class Camera;
    class Shader;
    class Texture2D;
    class TextureCube;

    class Renderer {
    public:
        static void Init();
        static void Shutdown();
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void BeginScene(EditorCamera& camera);
        static void BeginScene(Camera& camera, glm::mat4 transform);

        static void EndScene();

        static void Submit(const Ref<Shader>& shader,
                           const Ref<VertexArray>& vertexArray,
                           const glm::mat4& transform = glm::mat4(1.0f));

        static void DrawCube(const Ref<Shader>& shader, const glm::mat4& transform = glm::mat4(1.0f));
        static void DrawSkybox(Ref<TextureCube>& textureCube, EditorCamera& camera);
        static void DrawSkybox(Ref<TextureCube>& textureCube, Camera& camera, glm::mat4& transform);

        static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;

            Ref<VertexArray> CubeVertexArray;

            Ref<Texture2D> whiteTexture;
            Ref<Shader> SkyboxShader;
        };

        static Scope<SceneData> s_SceneData;
    };
}// namespace Aph

#endif// Aphrodite_ENGINE_RENDERER_H
