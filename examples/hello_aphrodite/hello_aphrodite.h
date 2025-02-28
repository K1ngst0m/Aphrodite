#pragma once

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class hello_aphrodite : public aph::BaseApp<hello_aphrodite>
{
public:
    hello_aphrodite();

    void toggleMeshShading(bool value, bool toggle = false);

private:
    void init() override;
    void load() override;
    void loop() override;
    void unload() override;
    void finish() override;

private:
    aph::vk::Buffer* m_pVB = {};
    aph::vk::Buffer* m_pIB = {};
    aph::vk::Buffer* m_pMatBuffer = {};
    aph::vk::Sampler* m_pSampler = {};
    aph::vk::Image* m_pImage = {};

    struct
    {
        aph::vk::ShaderProgram* geometry = {};
        aph::vk::ShaderProgram* mesh = {};
    } m_program;

private:
    std::unique_ptr<aph::Renderer> m_renderer = {};
    aph::ResourceLoader* m_pResourceLoader = {};
    aph::WindowSystem* m_pWindowSystem = {};
    aph::vk::Device* m_pDevice = {};
    aph::vk::SwapChain* m_pSwapChain = {};

    aph::Camera m_camera = { aph::CameraType::Perspective };

    bool m_enableMeshShading = true;

    struct
    {
        glm::mat4 model{ 1.0f };
        glm::mat4 view{ 1.0f };
        glm::mat4 proj{ 1.0f };
    } m_mvp;
};
