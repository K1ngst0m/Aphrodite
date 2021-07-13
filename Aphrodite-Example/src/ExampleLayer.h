//
// Created by npchitman on 7/3/21.
//

#ifndef APHRODITE_EXAMPLELAYER_H
#define APHRODITE_EXAMPLELAYER_H

#include <Aphrodite.hpp>

class ExampleLayer : public Aph::Layer {
public:
    ExampleLayer();
    ~ExampleLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(Aph::Timestep ts) override;
    void OnUIRender() override;
    void OnEvent(Aph::Event& e) override;

private:
    Aph::ShaderLibrary m_ShaderLibrary;
    Aph::Ref<Aph::Shader> m_Shader;
    Aph::Ref<Aph::VertexArray> m_VertexArray;

    Aph::Ref<Aph::Shader> m_FlatColorShader;
    Aph::Ref<Aph::VertexArray> m_SquareVA;

    Aph::Ref<Aph::Texture2D> m_Texture, m_ExampleTexture;

    Aph::OrthographicCameraController m_CameraController;
    glm::vec3 m_SquareColor = {0.2f, 0.3f, 0.8f};
};

#endif//APHRODITE_EXAMPLELAYER_H
