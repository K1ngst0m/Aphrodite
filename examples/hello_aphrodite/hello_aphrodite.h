#pragma once

#include "aph_core.hpp"

class HelloAphrodite : public aph::App
{
public:
    HelloAphrodite();

    enum class ShadingType
    {
        Geometry,
        Mesh,
        MeshBindless,
    };

    void switchShadingType(std::string_view value);
    void switchShadingType(ShadingType type);

private:
    // Core application lifecycle
    void init() override;
    void load() override;
    void loop() override;
    void unload() override;
    void finish() override;

    // Setup and rendering
    void setupEngine();
    void setupEventHandlers();
    void loadResources();
    void setupRenderGraph();
    void buildGraph(aph::RenderGraph* pGraph);
    void renderWithShadingType(aph::vk::CommandBuffer* pCmd, ShadingType type);

private:
    aph::vk::Buffer* m_pVertexBuffer = {};
    aph::vk::Buffer* m_pIndexBuffer = {};
    aph::vk::Buffer* m_pMatrixBffer = {};
    aph::vk::Sampler* m_pSampler = {};
    aph::vk::Image* m_pImage = {};

private:
    std::unique_ptr<aph::Engine> m_engine = {};
    aph::ResourceLoader* m_pResourceLoader = {};
    aph::WindowSystem* m_pWindowSystem = {};
    aph::vk::Device* m_pDevice = {};
    aph::vk::SwapChain* m_pSwapChain = {};

    aph::Camera m_camera = { aph::CameraType::Perspective };

    ShadingType m_shadingType = ShadingType::MeshBindless;
    aph::HashMap<ShadingType, aph::vk::ShaderProgram*> m_program;

    struct
    {
        aph::Mat4 model{ 1.0f };
        aph::Mat4 view{ 1.0f };
        aph::Mat4 proj{ 1.0f };
    } m_mvp;
};
