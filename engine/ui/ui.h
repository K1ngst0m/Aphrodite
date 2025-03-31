#pragma once

#include "common/enum.h"
#include "math/math.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

GENERATE_LOG_FUNCS(UI);

namespace aph::vk
{
class CommandBuffer;
}

namespace aph
{
// Forward declarations
class Renderer;
class WindowSystem;
class RenderBackend;

enum class UIFlagBits
{
    None = 0,
    Docking = 1 << 0,
    ViewportEnable = 1 << 1,
    Demo = 1 << 2,
    All = Docking | ViewportEnable | Demo
};

using UIFlags = Flags<UIFlagBits>;
template <>
struct FlagTraits<UIFlagBits>
{
    static constexpr bool isBitmask = true;
    static constexpr UIFlagBits allFlags = UIFlagBits::All;
};

struct UICreateInfo
{
    Renderer* pRenderer = nullptr;
    WindowSystem* pWindow = nullptr;
    UIFlags flags = UIFlagBits::None;
    std::string configFile = "";
};

// Abstract UI Backend interface
class UIBackend
{
public:
    virtual ~UIBackend() = default;
    virtual bool initialize(const UICreateInfo& createInfo) = 0;
    virtual void shutdown() = 0;
    virtual void newFrame() = 0;
    virtual void render(vk::CommandBuffer* pCmd) = 0;
};

// Main UI Manager class
class UI
{
public:
    using UIUpdateCallback = std::function<void()>;

    UI(const UICreateInfo& createInfo);
    ~UI();

    bool initialize();
    void shutdown();

    void beginFrame();
    void endFrame();
    void render(vk::CommandBuffer* pCmd);

    void setUpdateCallback(UIUpdateCallback&& callback);

    // Font handling
    uint32_t addFont(const std::string& fontPath, float fontSize);
    void setActiveFont(uint32_t fontIndex);

    template <typename TWidget>
    std::unique_ptr<TWidget> createWidget()
    {
        auto widget = std::make_unique<TWidget>(this);
        return widget;
    }

    // Add this method to access the backend
    UIBackend* getBackend() const
    {
        return m_backend.get();
    }

private:
    // Create the appropriate backend based on build configuration
    std::unique_ptr<UIBackend> createBackend();

    UICreateInfo m_createInfo;
    std::unique_ptr<UIBackend> m_backend;
    UIUpdateCallback m_updateCallback;
};

// Factory function
std::unique_ptr<UI> createUI(const UICreateInfo& createInfo);

} // namespace aph
