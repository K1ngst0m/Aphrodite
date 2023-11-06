#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "renderer/renderGraph/renderGraph.h"
#include "renderer/renderer.h"
#include "resource/resourceLoader.h"
#include "uiRenderer.h"

namespace aph::vk
{
class Renderer : public IRenderer
{
public:
    Renderer(WSI* wsi, const RenderConfig& config);
    ~Renderer() override;

    void nextFrame();

public:
    void load() override;
    void unload() override;
    void update(float deltaTime) override;

public:
    SwapChain*      getSwapchain() const { return m_pSwapChain; }
    ResourceLoader* getResourceLoader() const { return m_pResourceLoader.get(); }
    Instance*       getInstance() const { return m_pInstance; }
    Device*         getDevice() const { return m_pDevice.get(); }
    RenderGraph*    getGraph() { return m_frameGraph[m_frameIdx].get(); }
    UI*             getUI() const { return m_pUI; }

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

protected:
    std::vector<std::unique_ptr<RenderGraph>> m_frameGraph;
    uint32_t                                  m_frameIdx = {};
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
