#pragma once

#include <aphrodite.hpp>

class HelloAphrodite : public aph::App
{
public:
    HelloAphrodite();
    ~HelloAphrodite();

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

    // UI setup
    void setupUI();
    void setupCameraUI();
    void setupShaderDebugUI();

private:
    aph::Engine* m_pEngine                 = {};
    aph::ResourceLoader* m_pResourceLoader = {};
    aph::WindowSystem* m_pWindowSystem     = {};
    aph::UI* m_pUI                         = {};
    aph::FrameComposer* m_pFrameComposer;
    aph::vk::Device* m_pDevice       = {};
    aph::vk::SwapChain* m_pSwapChain = {};

    aph::Camera m_camera = {aph::CameraType::Perspective};

    struct
    {
        aph::Mat4 model{1.0f};
        aph::Mat4 view{1.0f};
        aph::Mat4 proj{1.0f};
    } m_mvp;

    // Camera UI widget
    aph::WidgetWindow* m_cameraWindow         = nullptr;
    aph::CameraControlWidget* m_cameraControl = nullptr;
    
    // Shader debug UI widget
    aph::WidgetWindow* m_shaderInfoWindow     = nullptr;
    aph::ShaderInfoWidget* m_shaderInfoWidget = nullptr;
};
