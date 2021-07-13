//
// Created by npchitman on 7/7/21.
//

#include "SceneRenderer.h"

#include "Aphrodite/Renderer/EditorCamera.h"
#include "Aphrodite/Renderer/Material.h"
#include "Aphrodite/Renderer/Model.h"
#include "Aphrodite/Renderer/RenderCommand.h"
#include "Aphrodite/Scene/Components.h"
#include "Aphrodite/Scene/Entity.h"
#include "glm/gtc/type_ptr.hpp"
#include "pch.h"

namespace Aph {
    Ref<UniformBuffer> SceneRenderer::m_UBOCamera;
    Ref<UniformBuffer> SceneRenderer::m_UBOLights;
    Ref<Shader> SceneRenderer::m_Shader;

    void SceneRenderer::Init() {
        m_UBOCamera = UniformBuffer::Create();
        m_UBOCamera->SetLayout({{ShaderDataType::Mat4, "u_View"},
                                {ShaderDataType::Mat4, "u_Projection"},
                                {ShaderDataType::Mat4, "u_ViewProjection"},
                                {ShaderDataType::Float4, "u_CameraPosition"}},
                               0);

        m_UBOLights = UniformBuffer::Create();
        m_UBOLights->SetLayout({
                                       {ShaderDataType::Float4, "u_Position"},
                                       {ShaderDataType::Float4, "u_Color"},
                                       {ShaderDataType::Float4, "u_AttenFactors"},
                                       {ShaderDataType::Float4, "u_LightDir"},
                                       {ShaderDataType::Float4, "u_Intensity"},
                               },
                               1, 26);// 25 is max number of lights

        m_Shader = Shader::Create("assets/shaders/PBR.glsl");
        m_Shader->Bind();
        m_Shader->SetUniformBlock("Camera", 0);
        m_Shader->SetUniformBlock("LightBuffer", 1);
    }

    void SceneRenderer::Shutdown() {
    }

    void SceneRenderer::OnWindowResize(uint32_t width, uint32_t height) {
        RenderCommand::SetViewport(0, 0, width, height);
    }

    void SceneRenderer::BeginScene(const EditorCamera& camera, std::vector<Entity>& lights) {
        m_UBOCamera->Bind();
        m_UBOCamera->SetData((void*) glm::value_ptr(camera.GetViewMatrix()), 0, sizeof(glm::mat4));
        m_UBOCamera->SetData((void*) glm::value_ptr(camera.GetProjection()), sizeof(glm::mat4), sizeof(glm::mat4));
        m_UBOCamera->SetData((void*) glm::value_ptr(camera.GetViewProjection()), 2 * sizeof(glm::mat4), sizeof(glm::mat4));
        m_UBOCamera->SetData((void*) glm::value_ptr(camera.GetPosition()), 3 * sizeof(glm::mat4), sizeof(glm::vec4));

        SetupLights(lights);
    }

    void SceneRenderer::BeginScene(const Camera& camera, const glm::mat4& cameraTransform, glm::vec3& cameraPosition, std::vector<Entity>& lights) {
        m_UBOCamera->Bind();

        glm::mat4 view = glm::inverse(cameraTransform);
        glm::mat4 projection = camera.GetProjection();

        m_UBOCamera->SetData((void*) glm::value_ptr(view), 0, sizeof(glm::mat4));
        m_UBOCamera->SetData((void*) glm::value_ptr(projection), sizeof(glm::mat4), sizeof(glm::mat4));
        m_UBOCamera->SetData((void*) glm::value_ptr(projection * view), 2 * sizeof(glm::mat4), sizeof(glm::mat4));
        m_UBOCamera->SetData((void*) glm::value_ptr(cameraPosition), 3 * sizeof(glm::mat4), sizeof(glm::vec4));

        SetupLights(lights);
    }

    void SceneRenderer::EndScene() { }

    void SceneRenderer::SubmitMesh(const Ref<Model>& mesh, const glm::mat4& transform,
                                   const Ref<MaterialInstance>& overrideMaterial) {
        std::vector<Mesh> submeshes = mesh->GetMeshes();
        for (uint32_t i = 0; i < submeshes.size(); i++) {
            Mesh submesh = submeshes.at(i);

            if (overrideMaterial) {
                overrideMaterial->GetShader()->Bind();
                overrideMaterial->GetShader()->SetMat4("u_Model", transform);
                overrideMaterial->Bind();
            } else {
                Ref<MaterialInstance> mat = mesh->GetMaterialInstance(i);
                mat->GetShader()->Bind();
                mat->GetShader()->SetMat4("u_Model", transform);
                mat->Bind();
            }

            submesh.meshVertexArray->Bind();
            RenderCommand::DrawIndexed(submesh.meshVertexArray);
        }
    }

    void SceneRenderer::SetupLights(std::vector<Entity>& lights) {
        m_UBOLights->Bind();
        uint32_t numLights = 0;
        uint32_t size = 5 * sizeof(glm::vec4);
        for (Entity e : lights) {
            TransformComponent transformComponent = e.GetComponent<TransformComponent>();
            LightComponent lightComponent = e.GetComponent<LightComponent>();

            glm::vec4 position = glm::vec4(transformComponent.Translation, 0.0f);

            glm::vec4 attenFactors = glm::vec4(
                    lightComponent.Constant,
                    lightComponent.Linear,
                    lightComponent.Quadratic,
                    static_cast<uint32_t>(lightComponent.Type));

            uint32_t offset = 0;
            m_UBOLights->SetData((void*) glm::value_ptr(position), size * numLights + offset, sizeof(glm::vec4));

            offset += sizeof(glm::vec4);
            m_UBOLights->SetData((void*) glm::value_ptr(lightComponent.Color), size * numLights + offset, sizeof(glm::vec4));

            offset += sizeof(glm::vec4);
            m_UBOLights->SetData((void*) glm::value_ptr(attenFactors), size * numLights + offset, sizeof(glm::vec4));

            glm::mat4 transform = transformComponent.GetTransform();
            offset += sizeof(glm::vec4);
            // Based off of -Z direction
            glm::vec4 zDir = transform * glm::vec4(0, 0, -1, 0);
            m_UBOLights->SetData((void*) glm::value_ptr(zDir), size * numLights + offset, sizeof(glm::vec4));

            offset += sizeof(glm::vec4);
            m_UBOLights->SetData(&lightComponent.Intensity, size * numLights + offset, sizeof(float));

            numLights++;
        }

        // Pass number of lights within the scene
        // 25 is max number of lights
        m_UBOLights->SetData(&numLights, 25 * size, sizeof(uint32_t));
    }
}// namespace Aph