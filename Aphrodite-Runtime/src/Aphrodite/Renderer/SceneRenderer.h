//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_SCENERENDERER_H
#define APHRODITE_SCENERENDERER_H

#include <glm/glm.hpp>

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Renderer/Buffer.h"
#include "Aphrodite/Renderer/Shader.h"

namespace Aph {
    class Camera;
    class TextureCube;
    class Entity;
    class EditorCamera;
    class MaterialInstance;
    class Mesh;
    class Scene;

    class SceneRenderer {
    public:
        static void Init();
        static void Shutdown();
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void BeginScene(const EditorCamera& camera, std::vector<Entity>& lights);
        static void BeginScene(const Camera& camera, const glm::mat4& cameraTransform, glm::vec3& cameraPosition, std::vector<Entity>& lights);
        static void EndScene();

        static void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, Ref<MaterialInstance> overrideMaterial = nullptr);

    private:
        static void SetupLights(std::vector<Entity>& lights);

    private:
        static Ref<UniformBuffer> m_UBOCamera;
        static Ref<UniformBuffer> m_UBOLights;
        static Ref<Shader> m_Shader;
    };
};// namespace Aph


#endif//APHRODITE_SCENERENDERER_H
