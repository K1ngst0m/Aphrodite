//
// Created by npchitman on 6/1/21.
//

#include "Aphrodite/Renderer/Renderer.h"

#include "Aphrodite/Renderer/Renderer2D.h"
#include "Aphrodite/Renderer/Shader.h"
#include "pch.h"

namespace Aph {
    Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

    void Renderer::Init() {
        APH_PROFILE_FUNCTION();

        RenderCommand::Init();
        Renderer2D::Init();

        // Skybox =========================================================================
        // Cube Data
        {
            float vertices[] = {
                    -0.5f, -0.5f, -0.5f,
                    0.5f, -0.5f, -0.5f,
                    0.5f, 0.5f, -0.5f,
                    0.5f, 0.5f, -0.5f,
                    -0.5f, 0.5f, -0.5f,
                    -0.5f, -0.5f, -0.5f,

                    -0.5f, -0.5f, 0.5f,
                    0.5f, -0.5f, 0.5f,
                    0.5f, 0.5f, 0.5f,
                    0.5f, 0.5f, 0.5f,
                    -0.5f, 0.5f, 0.5f,
                    -0.5f, -0.5f, 0.5f,

                    -0.5f, 0.5f, 0.5f,
                    -0.5f, 0.5f, -0.5f,
                    -0.5f, -0.5f, -0.5f,
                    -0.5f, -0.5f, -0.5f,
                    -0.5f, -0.5f, 0.5f,
                    -0.5f, 0.5f, 0.5f,

                    0.5f, 0.5f, 0.5f,
                    0.5f, 0.5f, -0.5f,
                    0.5f, -0.5f, -0.5f,
                    0.5f, -0.5f, -0.5f,
                    0.5f, -0.5f, 0.5f,
                    0.5f, 0.5f, 0.5f,

                    -0.5f, -0.5f, -0.5f,
                    0.5f, -0.5f, -0.5f,
                    0.5f, -0.5f, 0.5f,
                    0.5f, -0.5f, 0.5f,
                    -0.5f, -0.5f, 0.5f,
                    -0.5f, -0.5f, -0.5f,

                    -0.5f, 0.5f, -0.5f,
                    0.5f, 0.5f, -0.5f,
                    0.5f, 0.5f, 0.5f,
                    0.5f, 0.5f, 0.5f,
                    -0.5f, 0.5f, 0.5f,
                    -0.5f, 0.5f, -0.5f};

            s_SceneData->CubeVertexArray = VertexArray::Create();

            Ref<VertexBuffer> buffer = VertexBuffer::Create(vertices, sizeof(vertices));
            const BufferLayout layout{{ShaderDataType::Float3, "a_Position"}};
            buffer->SetLayout(layout);
            s_SceneData->CubeVertexArray->AddVertexBuffer(buffer);
            s_SceneData->SkyboxShader = Shader::Create("assets/shaders/Cubemap.glsl");
            s_SceneData->SkyboxShader->Bind();
            s_SceneData->SkyboxShader->SetInt("u_EnvironmentMap", 0);
        }
    }

    void Renderer::Shutdown() {
        Renderer2D::Shutdown();
    }

    void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
        RenderCommand::SetViewport(0, 0, width, height);
    }

    void Renderer::BeginScene(EditorCamera& camera) {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjection();
    }

    void Renderer::BeginScene(Camera& camera, glm::mat4 transform) {
        s_SceneData->ViewProjectionMatrix = camera.GetProjection() *
                                            glm::inverse(transform);
    }

    void Renderer::EndScene() {}

    void Renderer::Submit(const Ref<Shader>& shader,
                          const Ref<VertexArray>& vertexArray,
                          const glm::mat4& transform) {
        shader->Bind();
        shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
        shader->SetMat4("u_Transform", transform);

        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }

    void Renderer::DrawCube(const Ref<Shader>& shader, const glm::mat4& transform) {
        shader->Bind();
        shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
        shader->SetMat4("u_Transform", transform);

        s_SceneData->CubeVertexArray->Bind();
        RenderCommand::DrawArray(0, 36);
    }

    void Renderer::DrawSkybox(Ref<TextureCube>& textureCube, EditorCamera& camera) {
        RenderCommand::SetDepthMask(false);
        s_SceneData->SkyboxShader->Bind();
        s_SceneData->SkyboxShader->SetMat4("u_Projection", camera.GetProjection());
        s_SceneData->SkyboxShader->SetMat4("u_View", camera.GetViewMatrix());
        textureCube->Bind();
        s_SceneData->CubeVertexArray->Bind();
        RenderCommand::DrawArray(0, 36);
        RenderCommand::SetDepthMask(true);
    }

    void Renderer::DrawSkybox(Ref<TextureCube>& textureCube, Camera& camera, glm::mat4& transform) {
        RenderCommand::SetDepthMask(false);
        s_SceneData->SkyboxShader->Bind();
        s_SceneData->SkyboxShader->SetMat4("u_Projection", camera.GetProjection());
        s_SceneData->SkyboxShader->SetMat4("u_View", glm::inverse(transform));
        textureCube->Bind();
        s_SceneData->CubeVertexArray->Bind();
        RenderCommand::DrawArray(0, 36);
        RenderCommand::SetDepthMask(true);
    }

}// namespace Aph
