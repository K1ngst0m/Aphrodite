#pragma once

#include "api/vulkan/device.h"
#include "global/globalManager.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "ui/ui.h"

namespace aph
{
class EngineConfig
{
public:
    // Constructor with default values
    EngineConfig() = default;
    
    // Builder pattern methods
    EngineConfig& setMaxFrames(uint32_t maxFrames) 
    { 
        m_maxFrames = maxFrames; 
        return *this; 
    }
    
    EngineConfig& setWidth(uint32_t width) 
    { 
        m_width = width; 
        return *this; 
    }
    
    EngineConfig& setHeight(uint32_t height) 
    { 
        m_height = height; 
        return *this; 
    }
    
    EngineConfig& setWindowSystemCreateInfo(const WindowSystemCreateInfo& info)
    {
        m_windowSystemCreateInfo = info;
        return *this;
    }
    
    EngineConfig& setInstanceCreateInfo(const vk::InstanceCreateInfo& info)
    {
        m_instanceCreateInfo = info;
        return *this;
    }
    
    EngineConfig& setDeviceCreateInfo(const vk::DeviceCreateInfo& info)
    {
        m_deviceCreateInfo = info;
        return *this;
    }
    
    EngineConfig& setSwapChainCreateInfo(const vk::SwapChainCreateInfo& info)
    {
        m_swapChainCreateInfo = info;
        return *this;
    }
    
    EngineConfig& setResourceLoaderCreateInfo(const ResourceLoaderCreateInfo& info)
    {
        m_resourceLoaderCreateInfo = info;
        return *this;
    }
    
    EngineConfig& setUICreateInfo(const UICreateInfo& info)
    {
        m_uiCreateInfo = info;
        return *this;
    }
    
    // Getters
    uint32_t getMaxFrames() const { return m_maxFrames; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    
    const WindowSystemCreateInfo& getWindowSystemCreateInfo() const { return m_windowSystemCreateInfo; }
    const vk::InstanceCreateInfo& getInstanceCreateInfo() const { return m_instanceCreateInfo; }
    const vk::DeviceCreateInfo& getDeviceCreateInfo() const { return m_deviceCreateInfo; }
    const vk::SwapChainCreateInfo& getSwapChainCreateInfo() const { return m_swapChainCreateInfo; }
    const ResourceLoaderCreateInfo& getResourceLoaderCreateInfo() const { return m_resourceLoaderCreateInfo; }
    const UICreateInfo& getUICreateInfo() const { return m_uiCreateInfo; }

private:
    // Basic configuration
    uint32_t m_maxFrames = 2;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    
    // Create info structs for engine components
    WindowSystemCreateInfo m_windowSystemCreateInfo = {
        .width = 0,
        .height = 0,
        .enableUI = true
    };
    
    vk::InstanceCreateInfo m_instanceCreateInfo = {};
    
    vk::DeviceCreateInfo m_deviceCreateInfo = {
        .enabledFeatures = {
            .meshShading = true,
            .multiDrawIndirect = true,
            .tessellationSupported = true,
            .samplerAnisotropy = true,
            .rayTracing = false,
            .bindless = true,
        }
    };
    
    vk::SwapChainCreateInfo m_swapChainCreateInfo = {};
    
    ResourceLoaderCreateInfo m_resourceLoaderCreateInfo = {
        .async = true
    };
    
    UICreateInfo m_uiCreateInfo = {
        .flags = aph::UIFlagBits::Docking
    };
};

class Engine
{
private:
    Engine(const EngineConfig& config);

public:
    static std::unique_ptr<Engine> Create(const EngineConfig& config)
    {
        return std::unique_ptr<Engine>(new Engine(config));
    }
    ~Engine();

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
    EngineConfig m_config = {};

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