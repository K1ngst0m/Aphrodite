#include "engineConfig.h"

namespace aph
{

EngineConfig::EngineConfig(EngineConfigPreset preset)
{
    switch (preset)
    {
    case EngineConfigPreset::Default:
        // Default configuration with basic settings
        setWidth(1280)
            .setHeight(720)
            .setMaxFrames(2)
            .setEnableCapture(false)
            .setEnableDeviceInitLogs(false)
            .setEnableUIBreadcrumbs(false)
            .setResourceForceUncached(false)
            .setEnableResourceTracking(false);
        break;

    case EngineConfigPreset::Debug:
        // Debug configuration with additional logging and tools
        setWidth(1280)
            .setHeight(720)
            .setMaxFrames(2)
            .setEnableCapture(true)
            .setEnableDeviceInitLogs(true)
            .setEnableUIBreadcrumbs(true) // Enable breadcrumbs in debug mode
            .setResourceForceUncached(true) // Force resource reloading in debug mode
            .setEnableResourceTracking(true); // Enable resource tracking in debug mode
        break;

    case EngineConfigPreset::Headless:
        // Headless configuration without window
        setWidth(1)
            .setHeight(1)
            .setMaxFrames(1)
            .setEnableCapture(false)
            .setEnableDeviceInitLogs(false)
            .setEnableUIBreadcrumbs(false)
            .setResourceForceUncached(false)
            .setEnableResourceTracking(false);

        // Set window system with UI disabled
        WindowSystemCreateInfo windowInfo;
        windowInfo.width    = 1;
        windowInfo.height   = 1;
        windowInfo.enableUI = false;
        setWindowSystemCreateInfo(windowInfo);
        break;
    }
}

auto EngineConfig::setMaxFrames(uint32_t maxFrames) -> EngineConfig&
{
    m_maxFrames = maxFrames;
    return *this;
}

auto EngineConfig::setWidth(uint32_t width) -> EngineConfig&
{
    m_width = width;
    return *this;
}

auto EngineConfig::setHeight(uint32_t height) -> EngineConfig&
{
    m_height = height;
    return *this;
}

auto EngineConfig::setEnableCapture(bool value) -> EngineConfig&
{
    m_enableCapture = value;
    return *this;
}

auto EngineConfig::setEnableDeviceInitLogs(bool value) -> EngineConfig&
{
    m_enableDeviceInitLogs = value;
    return *this;
}

auto EngineConfig::setEnableUIBreadcrumbs(bool value) -> EngineConfig&
{
    m_enableUIBreadcrumbs = value;
    return *this;
}

auto EngineConfig::setWindowSystemCreateInfo(const WindowSystemCreateInfo& info) -> EngineConfig&
{
    m_windowSystemCreateInfo = info;
    return *this;
}

auto EngineConfig::setInstanceCreateInfo(const vk::InstanceCreateInfo& info) -> EngineConfig&
{
    m_instanceCreateInfo = info;
    return *this;
}

auto EngineConfig::setDeviceCreateInfo(const vk::DeviceCreateInfo& info) -> EngineConfig&
{
    m_deviceCreateInfo = info;
    return *this;
}

auto EngineConfig::setSwapChainCreateInfo(const vk::SwapChainCreateInfo& info) -> EngineConfig&
{
    m_swapChainCreateInfo = info;
    return *this;
}

auto EngineConfig::setResourceLoaderCreateInfo(const ResourceLoaderCreateInfo& info) -> EngineConfig&
{
    m_resourceLoaderCreateInfo = info;
    return *this;
}

auto EngineConfig::setResourceForceUncached(bool value) -> EngineConfig&
{
    m_resourceLoaderCreateInfo.forceUncached = value;
    return *this;
}

auto EngineConfig::setUICreateInfo(const UICreateInfo& info) -> EngineConfig&
{
    m_uiCreateInfo = info;
    return *this;
}

auto EngineConfig::setEnableResourceTracking(bool value) -> EngineConfig&
{
    m_enableResourceTracking = value;
    return *this;
}

auto EngineConfig::getMaxFrames() const -> uint32_t
{
    return m_maxFrames;
}

auto EngineConfig::getWidth() const -> uint32_t
{
    return m_width;
}

auto EngineConfig::getHeight() const -> uint32_t
{
    return m_height;
}

auto EngineConfig::getEnableCapture() const -> bool
{
    return m_enableCapture;
}

auto EngineConfig::getEnableDeviceInitLogs() const -> bool
{
    return m_enableDeviceInitLogs;
}

auto EngineConfig::getEnableUIBreadcrumbs() const -> bool
{
    return m_enableUIBreadcrumbs;
}

auto EngineConfig::getWindowSystemCreateInfo() const -> const WindowSystemCreateInfo&
{
    return m_windowSystemCreateInfo;
}

auto EngineConfig::getInstanceCreateInfo() const -> const vk::InstanceCreateInfo&
{
    return m_instanceCreateInfo;
}

auto EngineConfig::getDeviceCreateInfo() const -> const vk::DeviceCreateInfo&
{
    return m_deviceCreateInfo;
}

auto EngineConfig::getSwapChainCreateInfo() const -> const vk::SwapChainCreateInfo&
{
    return m_swapChainCreateInfo;
}

auto EngineConfig::getResourceLoaderCreateInfo() const -> const ResourceLoaderCreateInfo&
{
    return m_resourceLoaderCreateInfo;
}

auto EngineConfig::getUICreateInfo() const -> const UICreateInfo&
{
    return m_uiCreateInfo;
}

auto EngineConfig::getResourceForceUncached() const -> bool
{
    return m_resourceLoaderCreateInfo.forceUncached;
}

auto EngineConfig::getEnableResourceTracking() const -> bool
{
    return m_enableResourceTracking;
}

} // namespace aph