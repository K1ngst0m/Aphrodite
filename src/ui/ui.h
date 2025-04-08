#pragma once

#include "allocator/objectPool.h"
#include "common/enum.h"
#include "common/result.h"
#include "exception/errorMacros.h"
#include "math/math.h"

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
} // namespace aph::vk

namespace aph
{
// Forward declarations
class Engine;
class WindowSystem;
class WidgetContainer;
class WidgetWindow;
class Widget;

// Forward declare widget types (defined in widget.h)
enum class WidgetType;

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
private:
    UI(const UICreateInfo& createInfo);
    Result initialize(const UICreateInfo& createInfo);
    ~UI();

public:
    using UIUpdateCallback = std::function<void()>;

    // Factory methods
    static Expected<UI*> Create(const UICreateInfo& createInfo);
    static void Destroy(UI* pUI);

    void shutdown();

    void beginFrame();
    void endFrame();
    void render(vk::CommandBuffer* pCmd);

    void setUpdateCallback(UIUpdateCallback&& callback);

    // Font handling
    uint32_t addFont(const std::string& fontPath, float fontSize);
    void setActiveFont(uint32_t fontIndex);

    // Widget creation and management
    template <typename TWidget>
    TWidget* createWidget();
    void destroyWidget(Widget* widget);

    // Window creation and cleanup
    Expected<WidgetWindow*> createWindow(const std::string& title);
    void destroyWindow(WidgetWindow* window);

    // Container management
    void registerContainer(WidgetContainer* container);
    void unregisterContainer(WidgetContainer* container);
    void clearContainers();

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
    SmallVector<ImFont*> m_fonts;
    uint32_t m_activeFontIndex = 0;

    // Widget containers
    SmallVector<WidgetContainer*> m_containers;

    // Object pools for allocation
    ThreadSafeObjectPool<WidgetWindow> m_windowPool;
    ThreadSafePolymorphicObjectPool<Widget> m_widgetPool;

    // Config
    UICreateInfo m_createInfo;
    UIUpdateCallback m_updateCallback;
};

// Helper function for the UI class to create widgets easily
template <typename T>
T* UI::createWidget()
{
    return m_widgetPool.allocate<T>(this);
}

} // namespace aph
