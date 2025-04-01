#pragma once

#include "api/vulkan/device.h"
#include "global/globalManager.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "ui/ui.h"

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

public:
    vk::Instance* getInstance() const
    {
        return m_pInstance;
    }
    vk::SwapChain* getSwapchain() const
    {
        return m_pSwapChain;
    }
    UI* getUI() const
    {
        return m_ui.get();
    }
    ResourceLoader* getResourceLoader() const
    {
        return m_pResourceLoader.get();
    }
    vk::Device* getDevice() const
    {
        return m_pDevice.get();
    }
    WindowSystem* getWindowSystem() const
    {
        return m_pWindowSystem.get();
    }

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

public:
    coro::generator<RenderGraph*> setupGraph();

    struct FrameResource
    {
        RenderGraph* pGraph;
        uint32_t frameIdx;
    };
    coro::generator<FrameResource> loop();

private:
    void update();
    void render();
    SmallVector<std::unique_ptr<RenderGraph>> m_frameGraph;

protected:
    RenderConfig m_config = {};

protected:
    uint32_t m_frameIdx = {};

protected:
    vk::Instance* m_pInstance = {};
    vk::SwapChain* m_pSwapChain = {};
    std::unique_ptr<ResourceLoader> m_pResourceLoader;
    std::unique_ptr<vk::Device> m_pDevice = {};
    std::unique_ptr<WindowSystem> m_pWindowSystem = {};
    std::unique_ptr<UI> m_ui{};
    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;

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
