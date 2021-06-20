//
// Created by npchitman on 6/1/21.
//

#include "Renderer.h"
#include "hzpch.h"

namespace Hazel {
Renderer::SceneData *Renderer::s_SceneData = new Renderer::SceneData;

void Renderer::BeginScene(OrthographicCamera &camera) {
  s_SceneData->ViewProjectionMatrix = camera.GetProjectionMatrix();
}

void Renderer::EndScene() {}

void Renderer::Submit(const std::shared_ptr<Shader> &shader,
                      const std::shared_ptr<VertexArray> &vertexArray) {
  shader->Bind();
  shader->UploadUniformMat4("u_ViewProjection",
                            s_SceneData->ViewProjectionMatrix);

  vertexArray->Bind();
  RenderCommand::DrawIndexed(vertexArray);
}
} // namespace Hazel
