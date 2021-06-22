//
// Created by npchitman on 6/21/21.
//

#ifndef HAZEL_ENGINE_SANDBOX2D_H
#define HAZEL_ENGINE_SANDBOX2D_H

#include "Hazel.h"

class Sandbox2D : public Hazel::Layer {
public:
    Sandbox2D();
    ~Sandbox2D() override = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(Hazel::Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(Hazel::Event& e) override;

private:
    Hazel::OrthographicCameraController m_CameraController;

    Hazel::Ref<Hazel::VertexArray> m_SquareVA;
    Hazel::Ref<Hazel::Shader> m_FlatColorShader;

    glm::vec4 m_SquareColor{0.2f, 0.3f, 0.8f, 1.0f};

    Hazel::Ref<Hazel::Texture2D> m_CheckerboardTexture;
};


#endif//HAZEL_ENGINE_SANDBOX2D_H
