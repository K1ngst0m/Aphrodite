//
// Created by npchitman on 7/3/21.
//

#ifndef APHRODITE_SANDBOX2D_H
#define APHRODITE_SANDBOX2D_H

#include <Aphrodite.hpp>

class Sandbox2D : public Aph::Layer {
public:
    Sandbox2D();
    ~Sandbox2D() override = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(Aph::Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(Aph::Event& e) override;

private:
    Aph::OrthographicCameraController m_CameraController;

    // Temp
    Aph::Ref<Aph::VertexArray> m_SquareVA;
    Aph::Ref<Aph::Shader> m_FlatColorShader;

    Aph::Ref<Aph::Texture2D> m_CheckerboardTexture;

    glm::vec4 m_SquareColor = {0.2f, 0.3f, 0.8f, 1.0f};
};

#endif//APHRODITE_SANDBOX2D_H
