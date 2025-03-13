#pragma once

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class hello_aphrodite : public aph::BaseApp<hello_aphrodite>
{
public:
    hello_aphrodite();

    enum class ShadingType
    {
        Geometry,
        Mesh,
        MeshBindless,
    };

    void switchShadingType(std::string_view value);
    void switchShadingType(ShadingType type);

private:
    void init() override;
    void load() override;
    void loop() override;
    void unload() override;
    void finish() override;

private:
    aph::vk::Buffer* m_pVertexBuffer = {};
    aph::vk::Buffer* m_pIndexBuffer = {};
    aph::vk::Buffer* m_pMatrixBffer = {};
    aph::vk::Sampler* m_pSampler = {};
    aph::vk::Image* m_pImage = {};

    uint32_t m_drawDataOffset = 0;

private:
    std::unique_ptr<aph::Renderer> m_renderer = {};
    aph::ResourceLoader* m_pResourceLoader = {};
    aph::WindowSystem* m_pWindowSystem = {};
    aph::vk::Device* m_pDevice = {};
    aph::vk::SwapChain* m_pSwapChain = {};

    aph::Camera m_camera = { aph::CameraType::Perspective };

    ShadingType m_shadingType = ShadingType::MeshBindless;
    aph::HashMap<ShadingType, aph::vk::ShaderProgram*> m_program;

    struct
    {
        glm::mat4 model{ 1.0f };
        glm::mat4 view{ 1.0f };
        glm::mat4 proj{ 1.0f };
    } m_mvp;
};
