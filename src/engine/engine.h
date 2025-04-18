#pragma once

#include "api/capture.h"
#include "api/vulkan/device.h"
#include "engineConfig.h"
#include "global/globalManager.h"
#include "material/materialRegistry.h"
#include "renderGraph/frameComposer.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "ui/ui.h"

namespace aph
{
class Engine
{
public:
    // Factory methods
    static auto Create(const EngineConfig& config) -> Expected<Engine*>;
    static void Destroy(Engine* pEngine);

    // Main frame loop for the engine
    // Returns a generator yielding the current frame's resources
    auto loop() -> coro::generator<FrameComposer::FrameResource>;

    // Core accessors
    auto getInstance() const -> vk::Instance*;
    auto getDevice() const -> vk::Device*;
    auto getSwapchain() const -> vk::SwapChain*;
    auto getWindowSystem() const -> WindowSystem*;
    auto getConfig() const -> const EngineConfig&;
    auto getFrameComposer() const -> FrameComposer*;
    auto getResourceLoader() const -> ResourceLoader*;
    auto getUI() const -> UI*;
    auto getDeviceCapture() -> DeviceCapture*;
    auto getMaterialRegistry() const -> MaterialRegistry*;

    // Performance metrics
    auto getElapsedTime() const -> double;
    auto getCPUFrameTime() const -> double;
    auto getResourceForceUncached() const -> bool;

    // Window and framebuffer dimensions
    auto getWindowWidth() const -> uint32_t;
    auto getWindowHeight() const -> uint32_t;
    auto getPixelWidth() const -> uint32_t;
    auto getPixelHeight() const -> uint32_t;
    auto getDPIScale() const -> float;
    auto isHighDPIEnabled() const -> bool;

    // Structure to pass to debug callback
    struct DebugCallbackData
    {
        uint32_t frameId;
        bool enableDeviceInitLogs;
    };

    // Deleted constructors and assignment operators
    Engine(const Engine&)                    = delete;
    Engine(Engine&&)                         = delete;
    auto operator=(const Engine&) -> Engine& = delete;
    auto operator=(Engine&&) -> Engine&      = delete;

private:
    // Private constructor and initialization
    explicit Engine(const EngineConfig& config);
    auto initialize(const EngineConfig& config) -> Result;
    ~Engine() = default;

    // Frame processing
    void update();
    void render();

    // Core components
    vk::Instance* m_pInstance         = {};
    vk::SwapChain* m_pSwapChain       = {};
    vk::Device* m_pDevice             = {};
    WindowSystem* m_pWindowSystem     = {};
    ResourceLoader* m_pResourceLoader = nullptr;
    FrameComposer* m_pFrameComposer   = nullptr;
    UI* m_ui                          = nullptr;
    DeviceCapture* m_pDeviceCapture   = nullptr;
    MaterialRegistry* m_pMaterialRegistry = nullptr;

    // Task management and debugging
    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;
    DebugCallbackData m_debugCallbackData{};

    // Timing and configuration
    enum TimerTag : uint8_t
    {
        eTimerTagGlobal,
        eTimerTagFrame,
    };
    aph::Timer m_timer;
    double m_frameCPUTime;
    EngineConfig m_config;
};
} // namespace aph
