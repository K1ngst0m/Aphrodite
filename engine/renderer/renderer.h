#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "uiRenderer.h"

namespace aph
{

enum RenderConfigFlagBits
{
    RENDER_CFG_DEBUG       = (1 << 0),
    RENDER_CFG_UI          = (1 << 1),
    RENDER_CFG_DEFAULT_RES = (1 << 2),
    RENDER_CFG_WITHOUT_UI  = RENDER_CFG_DEBUG | RENDER_CFG_DEFAULT_RES,
    RENDER_CFG_ALL         = RENDER_CFG_DEFAULT_RES | RENDER_CFG_UI
#if defined(APH_DEBUG)
                     | RENDER_CFG_DEBUG
#endif
    ,
};
using RenderConfigFlags = uint32_t;

struct RenderConfig
{
    RenderConfigFlags flags     = RENDER_CFG_ALL;
    uint32_t          maxFrames = {2};
    uint32_t          width;
    uint32_t          height;
};
}  // namespace aph

namespace aph::vk
{
class Renderer
{
private:
    Renderer(const RenderConfig& config);

public:
    static std::unique_ptr<Renderer> Create(const RenderConfig& config)
    {
        return std::unique_ptr<Renderer>(new Renderer(config));
    }
    ~Renderer();

    void nextFrame();

public:
    void load();
    void unload();
    void update(float deltaTime);

public:
    SwapChain*      getSwapchain() const { return m_pSwapChain; }
    ResourceLoader* getResourceLoader() const { return m_pResourceLoader.get(); }
    Instance*       getInstance() const { return m_pInstance; }
    Device*         getDevice() const { return m_pDevice.get(); }
    RenderGraph*    getGraph() { return m_frameGraph[m_frameIdx].get(); }
    UI*             getUI() const { return m_pUI; }
    WSI*            getWSI() const { return m_wsi.get(); }

    const RenderConfig& getConfig() const { return m_config; }

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance*                       m_pInstance  = {};
    SwapChain*                      m_pSwapChain = {};
    std::unique_ptr<ResourceLoader> m_pResourceLoader;
    std::unique_ptr<Device>         m_pDevice = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

protected:
    UI* m_pUI = {};

    RenderConfig m_config = {};

    std::unique_ptr<WSI> m_wsi = {};

protected:
    std::vector<std::unique_ptr<RenderGraph>> m_frameGraph;
    uint32_t                                  m_frameIdx = {};
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
