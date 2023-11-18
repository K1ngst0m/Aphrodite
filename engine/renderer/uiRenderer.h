#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include "api/vulkan/device.h"
#include "resource/resourceLoader.h"

class ImGuiContext;
class ImFont;

namespace aph::vk
{

class Renderer;

enum UIFlags
{
    UI_Docking,
    UI_Demo,
};
MAKE_ENUM_FLAG(uint32_t, UIFlags);

struct UICreateInfo
{
    Renderer*   pRenderer  = {};
    UIFlags     flags      = {};
    std::string configFile = {};
};

class UI
{
public:
    UI(const UICreateInfo& ci);
    ~UI();

    using UIUpdateCallback = std::function<void()>;
    void record(UIUpdateCallback&& func) { m_upateCB = std::move(func); }
    void update();
    void load();
    void unload();

    void draw(CommandBuffer* pCmd);

public:
    uint32_t addFont(std::string_view fontPath, float pixelSize);
    void     pushFont(uint32_t id) const;
    void     popFont() const;

private:
    WSI*          m_pWSI     = {};
    ImGuiContext* m_pContext = {};

    UIUpdateCallback m_upateCB = {};

private:
    Renderer* m_pRenderer = {};
    Device*   m_pDevice   = {};

    VkDescriptorPool m_pool = {};

    Queue* m_pDefaultQueue = {};

    bool     m_showDemoWindow = false;

    std::vector<ImFont*> m_fonts;
};
}  // namespace aph::vk

#endif  // UIRENDERER_H_
