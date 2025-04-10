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
    explicit EngineConfig(EngineConfigPreset preset)
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
                .setEnableUIBreadcrumbs(false);
            break;

        case EngineConfigPreset::Debug:
            // Debug configuration with additional logging and tools
            setWidth(1280)
                .setHeight(720)
                .setMaxFrames(2)
                .setEnableCapture(true)
                .setEnableDeviceInitLogs(true)
                .setEnableUIBreadcrumbs(true); // Enable breadcrumbs in debug mode
            break;

        case EngineConfigPreset::Headless:
            // Headless configuration without window
            setWidth(1)
                .setHeight(1)
                .setMaxFrames(1)
                .setEnableCapture(false)
                .setEnableDeviceInitLogs(false)
                .setEnableUIBreadcrumbs(false);

            // Set window system with UI disabled
            WindowSystemCreateInfo windowInfo;
            windowInfo.width    = 1;
            windowInfo.height   = 1;
            windowInfo.enableUI = false;
            setWindowSystemCreateInfo(windowInfo);
            break;
        }
    }

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

    EngineConfig& setEnableCapture(bool value)
    {
        m_enableCapture = value;
        return *this;
    }

    EngineConfig& setEnableDeviceInitLogs(bool value)
    {
        m_enableDeviceInitLogs = value;
        return *this;
    }

    EngineConfig& setEnableUIBreadcrumbs(bool value)
    {
        m_enableUIBreadcrumbs = value;
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
    uint32_t getMaxFrames() const
    {
        return m_maxFrames;
    }
    uint32_t getWidth() const
    {
        return m_width;
    }
    uint32_t getHeight() const
    {
        return m_height;
    }

    bool getEnableCapture() const
    {
        return m_enableCapture;
    }

    bool getEnableDeviceInitLogs() const
    {
        return m_enableDeviceInitLogs;
    }

    bool getEnableUIBreadcrumbs() const
    {
        return m_enableUIBreadcrumbs;
    }

    const WindowSystemCreateInfo& getWindowSystemCreateInfo() const
    {
        return m_windowSystemCreateInfo;
    }
    const vk::InstanceCreateInfo& getInstanceCreateInfo() const
    {
        return m_instanceCreateInfo;
    }
    const vk::DeviceCreateInfo& getDeviceCreateInfo() const
    {
        return m_deviceCreateInfo;
    }
    const vk::SwapChainCreateInfo& getSwapChainCreateInfo() const
    {
        return m_swapChainCreateInfo;
    }
    const ResourceLoaderCreateInfo& getResourceLoaderCreateInfo() const
    {
        return m_resourceLoaderCreateInfo;
    }
    const UICreateInfo& getUICreateInfo() const
    {
        return m_uiCreateInfo;
    }

private:
    // Basic configuration
    uint32_t m_maxFrames        = 2;
    uint32_t m_width            = 0;
    uint32_t m_height           = 0;
    bool m_enableCapture        = false;
    bool m_enableDeviceInitLogs = false;
    bool m_enableUIBreadcrumbs  = false;

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

    ResourceLoaderCreateInfo m_resourceLoaderCreateInfo = {.async = true};

    UICreateInfo m_uiCreateInfo = {.flags = aph::UIFlagBits::Docking};
};

} // namespace aph
