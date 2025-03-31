#pragma once

#include "ui.h"

// Forward declarations for ImGui
struct ImGuiContext;
struct ImFont;
namespace aph
{
class Renderer;
class WindowSystem;

namespace vk
{
class Device;
class Instance;
class Queue;
class CommandBuffer;
} // namespace vk
} // namespace aph

namespace aph
{

// ImGui backend implementation
class ImGuiBackend : public UIBackend
{
public:
    ~ImGuiBackend() override = default;

    bool initialize(const UICreateInfo& createInfo) override;
    void shutdown() override;
    void newFrame() override;
    void render(vk::CommandBuffer* pCmd) override;

    uint32_t addFont(const std::string& fontPath, float fontSize);
    void setActiveFont(uint32_t fontIndex);
    void showDemoWindow(bool show);

private:
    // ImGui context
    ImGuiContext* m_context = nullptr;

    // Window reference
    WindowSystem* m_window = nullptr;

    // Vulkan resources
    vk::Device* m_device = nullptr;
    vk::Instance* m_instance = nullptr;
    vk::Queue* m_graphicsQueue = nullptr;
    Renderer* m_renderer = nullptr;

    // Font handling
    std::vector<ImFont*> m_fonts;
    uint32_t m_activeFontIndex = 0;

    // Config
    std::string m_configFile;
    UIFlags m_flags;

    // Demo window
    bool m_showDemoWindow = true;
};

} // namespace aph
