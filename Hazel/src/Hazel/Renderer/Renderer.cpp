//
// Created by npchitman on 6/1/21.
//

#include "Hazel/Renderer/Renderer.h"

#include "Hazel/Renderer/Renderer2D.h"
#include "hzpch.h"

namespace Hazel {
    Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

    void Renderer::Init() {
        RenderCommand::Init();
        Renderer2D::Init();
    }

    void Renderer::Shutdown() {
        Renderer2D::Shutdown();
    }

    void Renderer::BeginScene(OrthographicCamera &camera) {
        s_SceneData->ViewProjectionMatrix = camera.GetProjectionMatrix();
    }

    void Renderer::EndScene() {}

    void Renderer::Submit(const Ref<Shader> &shader, const Ref<VertexArray> &vertexArray, const glm::mat4 &transform) {
        shader->Bind();
        shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
        shader->SetMat4("u_transform", transform);

        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }
    void Renderer::OnWindowResize(uint32_t width, uint32_t height) {
        RenderCommand::SetViewport(0, 0, width, height);
    }
}// namespace Hazel
