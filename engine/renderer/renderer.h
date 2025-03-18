#pragma once

#include "api/vulkan/device.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "uiRenderer.h"

namespace aph
{
struct RenderConfig
{
    uint32_t maxFrames = { 2 };
    uint32_t width;
    uint32_t height;
};

class Renderer
{
private:
    Renderer(const RenderConfig& config);

public:
    static std::unique_ptr<Renderer> Create(const RenderConfig& config)
    {
        return std::unique_ptr<Renderer>(new Renderer(config));
    }
    ~Renderer();

public:
    void load();
    void unload();
    void update();

public:
    vk::Instance* getInstance() const
    {
        return m_pInstance;
    }
    vk::SwapChain* getSwapchain() const
    {
        return m_pSwapChain;
    }
    ResourceLoader* getResourceLoader() const
    {
        return m_pResourceLoader.get();
    }
    vk::Device* getDevice() const
    {
        return m_pDevice.get();
    }
    vk::UI* getUI() const
    {
        return m_pUI.get();
    }
    WindowSystem* getWindowSystem() const
    {
        return m_pWindowSystem.get();
    }

    Generator<RenderGraph*> recordGraph();
    void render();

    const RenderConfig& getConfig() const
    {
        return m_config;
    }

    double getElapsedTime() const
    {
        return m_timer.interval(TIMER_TAG_GLOBAL);
    }
    double getCPUFrameTime() const
    {
        return m_frameCPUTime;
    }

protected:
    RenderConfig m_config = {};

protected:
    SmallVector<std::unique_ptr<RenderGraph>> m_frameGraph;
    uint32_t m_frameIdx = {};

protected:
    vk::Instance* m_pInstance = {};
    vk::SwapChain* m_pSwapChain = {};
    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;
    std::unique_ptr<ResourceLoader> m_pResourceLoader;
    std::unique_ptr<vk::Device> m_pDevice = {};
    std::unique_ptr<vk::UI> m_pUI = {};
    std::unique_ptr<WindowSystem> m_pWindowSystem = {};

private:
    enum TimerTag
    {
        TIMER_TAG_GLOBAL,
        TIMER_TAG_FRAME,
    };
    aph::Timer m_timer;
    double m_frameCPUTime;
};
} // namespace aph
