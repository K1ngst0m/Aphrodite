#pragma once

#include "api/vulkan/device.h"
#include "resource/resourceLoader.h"

struct ImGuiContext;
struct ImFont;
namespace aph
{
class Renderer;
}

namespace aph::vk
{

enum UIFlags
{
    UI_Docking,
    UI_Demo,
};

struct UICreateInfo
{
    aph::Renderer* pRenderer = {};
    UIFlags flags = {};
    std::string configFile = {};
};

class UI
{
public:
    UI(const UICreateInfo& ci);
    ~UI();

    using UIUpdateCallback = std::function<void()>;
    void record(UIUpdateCallback&& func)
    {
        m_upateCB = std::move(func);
    }
    void update();
    void load();
    void unload();

    void draw(CommandBuffer* pCmd);

public:
    uint32_t addFont(std::string_view fontPath, float pixelSize);

private:
    WindowSystem* m_pWSI = {};
    ImGuiContext* m_pContext = {};

    UIUpdateCallback m_upateCB = {};

private:
    Renderer* m_pRenderer = {};
    Device* m_pDevice = {};

    VkDescriptorPool m_pool = {};

    Queue* m_pDefaultQueue = {};

    SmallVector<ImFont*> m_fonts;
};
} // namespace aph::vk
