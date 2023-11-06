#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include "api/vulkan/device.h"
#include "resource/resourceLoader.h"

class ImGuiContext;

namespace aph::vk
{

class Renderer;

enum UIFlags
{
    UI_Docking,
    UI_Demo,
};

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

    void update();
    void load();
    void unload();

    void draw(CommandBuffer* pCmd);

private:
    WSI*          m_pWSI     = {};
    ImGuiContext* m_pContext = {};

    bool m_updated = {false};

private:
    Renderer* m_pRenderer = {};
    Device*   m_pDevice   = {};

    VkDescriptorPool m_pool = {};

    Queue* m_pDefaultQueue = {};

    uint32_t m_frameIdx       = 0;
    bool     m_showDemoWindow = false;

#if 0
private:
    VertexInput m_vertexInput = {};
    DescriptorSet*       m_pSet            = {};
    Image*               m_pFontImage      = {};
    Sampler*             m_pDefaultSampler = {};
    Pipeline*            m_pPipeline       = {};
    Buffer*              m_pVertexBuffer   = {};
    Buffer*              m_pIndexBuffer    = {};
    std::vector<Buffer*> m_pUniformBuffers = {};
#endif
};
}  // namespace aph::vk

#endif  // UIRENDERER_H_
