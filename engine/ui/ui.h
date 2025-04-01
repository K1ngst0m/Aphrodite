#pragma once

#include "common/enum.h"
#include "math/math.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

GENERATE_LOG_FUNCS(UI);

// Forward declarations for ImGui
struct ImGuiContext;
struct ImFont;

namespace aph::vk
{
class CommandBuffer;
class Device;
class SwapChain;
class Instance;
class Queue;
}

namespace aph
{
// Forward declarations
class Renderer;
class WindowSystem;

enum class UIFlagBits
{
    None = 0,
    Docking = 1 << 0,
    ViewportEnable = 1 << 1,
    All = Docking | ViewportEnable
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
    vk::Instance* pInstance = {};
    vk::Device* pDevice = {};
    vk::SwapChain* pSwapchain = {};
    WindowSystem* pWindow = {};
    UIFlags flags = UIFlagBits::None;
    std::string configFile = "";
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

private:
    // ImGui context
    ImGuiContext* m_context = {};

    // Window reference
    WindowSystem* m_window = {};

    // Vulkan resources
    vk::Device* m_device = {};
    vk::Instance* m_instance = {};
    vk::Queue* m_graphicsQueue = {};
    vk::SwapChain* m_swapchain = {};

    // Font handling
    std::vector<ImFont*> m_fonts;
    uint32_t m_activeFontIndex = 0;

    // Config
    UICreateInfo m_createInfo;
    UIUpdateCallback m_updateCallback;
};

// Factory function
std::unique_ptr<UI> createUI(const UICreateInfo& createInfo);

} // namespace aph
