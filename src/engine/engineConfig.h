#pragma once

#include "api/vulkan/device.h"
#include "resource/resourceLoader.h"
#include "ui/ui.h"

namespace aph
{
// Configuration types for engine creation
enum class EngineConfigPreset
{
    Default,
    Debug,
    Headless
};

class EngineConfig
{
public:
    // Constructor with default values
    EngineConfig() = default;

    // Constructor with preset configuration
    explicit EngineConfig(EngineConfigPreset preset);

    // Builder pattern setters
    auto setMaxFrames(uint32_t maxFrames) -> EngineConfig&;
    auto setWidth(uint32_t width) -> EngineConfig&;
    auto setHeight(uint32_t height) -> EngineConfig&;
    auto setEnableCapture(bool value) -> EngineConfig&;
    auto setEnableDeviceInitLogs(bool value) -> EngineConfig&;
    auto setEnableUIBreadcrumbs(bool value) -> EngineConfig&;
    auto setWindowSystemCreateInfo(const WindowSystemCreateInfo& info) -> EngineConfig&;
    auto setInstanceCreateInfo(const vk::InstanceCreateInfo& info) -> EngineConfig&;
    auto setDeviceCreateInfo(const vk::DeviceCreateInfo& info) -> EngineConfig&;
    auto setSwapChainCreateInfo(const vk::SwapChainCreateInfo& info) -> EngineConfig&;
    auto setResourceLoaderCreateInfo(const ResourceLoaderCreateInfo& info) -> EngineConfig&;
    auto setResourceForceUncached(bool value) -> EngineConfig&;
    auto setUICreateInfo(const UICreateInfo& info) -> EngineConfig&;
    auto setEnableResourceTracking(bool value) -> EngineConfig&;

    // Getters
    auto getMaxFrames() const -> uint32_t;
    auto getWidth() const -> uint32_t;
    auto getHeight() const -> uint32_t;
    auto getEnableCapture() const -> bool;
    auto getEnableDeviceInitLogs() const -> bool;
    auto getEnableUIBreadcrumbs() const -> bool;
    auto getWindowSystemCreateInfo() const -> const WindowSystemCreateInfo&;
    auto getInstanceCreateInfo() const -> const vk::InstanceCreateInfo&;
    auto getDeviceCreateInfo() const -> const vk::DeviceCreateInfo&;
    auto getSwapChainCreateInfo() const -> const vk::SwapChainCreateInfo&;
    auto getResourceLoaderCreateInfo() const -> const ResourceLoaderCreateInfo&;
    auto getUICreateInfo() const -> const UICreateInfo&;
    auto getResourceForceUncached() const -> bool;
    auto getEnableResourceTracking() const -> bool;

private:
    // Basic configuration
    uint32_t m_maxFrames        = 2;
    uint32_t m_width            = 0;
    uint32_t m_height           = 0;
    bool m_enableCapture        = false;
    bool m_enableDeviceInitLogs = false;
    bool m_enableUIBreadcrumbs  = false;
    bool m_enableResourceTracking = false; // Track resource creation/destruction

    // Create info structs for engine components
    WindowSystemCreateInfo m_windowSystemCreateInfo = {.width = 0, .height = 0, .enableUI = true};

    vk::InstanceCreateInfo m_instanceCreateInfo = {};

    vk::DeviceCreateInfo m_deviceCreateInfo = {
        .enabledFeatures = {
                            .meshShading           = true,
                            .multiDrawIndirect     = true,
                            .tessellationSupported = true,
                            .samplerAnisotropy     = true,
                            .rayTracing            = false,
                            .bindless              = true,
                            }
    };

    vk::SwapChainCreateInfo m_swapChainCreateInfo = {};

    ResourceLoaderCreateInfo m_resourceLoaderCreateInfo = {.async = true, .forceUncached = false};

    UICreateInfo m_uiCreateInfo = {.flags = aph::UIFlagBits::Docking};
};

} // namespace aph
