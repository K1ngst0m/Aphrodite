#pragma once

#include "api/capture.h"
#include "api/vulkan/device.h"
#include "engineConfig.h"
#include "exception/errorMacros.h"
#include "global/globalManager.h"
#include "renderGraph/frameComposer.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "ui/ui.h"

namespace aph
{
class Engine
{
private:
    // Private constructor - use static Create methods instead
    Engine(const EngineConfig& config);
    Result initialize(const EngineConfig& config);
    ~Engine() = default;

public:
    // Structure to pass to debug callback
    struct DebugCallbackData
    {
        uint32_t frameId;
        bool enableDeviceInitLogs;
    };

    // Factory methods
    static Expected<Engine*> Create(const EngineConfig& config);
    static void Destroy(Engine* pEngine);

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
        return m_ui;
    }
    FrameComposer* getFrameComposer() const
    {
        return m_pFrameComposer;
    }
    ResourceLoader* getResourceLoader() const
    {
        return m_pResourceLoader;
    }
    vk::Device* getDevice() const
    {
        return m_pDevice;
    }
    WindowSystem* getWindowSystem() const
    {
        return m_pWindowSystem;
    }

    const EngineConfig& getConfig() const
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

    DeviceCapture* getDeviceCapture()
    {
        return m_pDeviceCapture;
    }

public:
    // Main frame loop for the engine
    // Returns a generator yielding the current frame's resources
    coro::generator<FrameComposer::FrameResource> loop();

private:
    void update();
    void render();

protected:
    vk::Instance* m_pInstance         = {};
    vk::SwapChain* m_pSwapChain       = {};
    vk::Device* m_pDevice             = {};
    WindowSystem* m_pWindowSystem     = {};
    ResourceLoader* m_pResourceLoader = nullptr;
    FrameComposer* m_pFrameComposer   = nullptr;
    UI* m_ui                          = nullptr;
    DeviceCapture* m_pDeviceCapture   = nullptr;

    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;
    DebugCallbackData m_debugCallbackData{};

private:
    enum TimerTag
    {
        TIMER_TAG_GLOBAL,
        TIMER_TAG_FRAME,
    };
    aph::Timer m_timer;
    double m_frameCPUTime;
    EngineConfig m_config = {};
};
} // namespace aph
