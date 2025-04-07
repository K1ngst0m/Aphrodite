#pragma once

#include "aph_core.hpp"

class HelloAphrodite : public aph::App
{
public:
    HelloAphrodite();

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

private:
    aph::BufferAsset* m_pVertexBuffer = {};
    aph::BufferAsset* m_pIndexBuffer = {};
    aph::BufferAsset* m_pMatrixBffer = {};
    aph::vk::Sampler* m_pSampler = {};
    aph::ImageAsset* m_pImageAsset = {};

private:
    aph::Engine* m_pEngine = {};
    aph::ResourceLoader* m_pResourceLoader = {};
    aph::WindowSystem* m_pWindowSystem = {};
    aph::vk::Device* m_pDevice = {};
    aph::vk::SwapChain* m_pSwapChain = {};

    aph::Camera m_camera = {aph::CameraType::Perspective};

    aph::ShaderAsset* m_pProgram = {};

    struct
    {
        aph::Mat4 model{1.0f};
        aph::Mat4 view{1.0f};
        aph::Mat4 proj{1.0f};
    } m_mvp;
};
