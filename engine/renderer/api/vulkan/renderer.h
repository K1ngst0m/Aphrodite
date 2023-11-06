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
    std::unique_ptr<ResourceLoader> m_pResourceLoader;
    std::unique_ptr<Device>         m_pDevice    = {};
    SwapChain*                      m_pSwapChain = {};

public:
    Shader*   getShaders(const std::filesystem::path& path) const;
    Queue*    getDefaultQueue(QueueType type) const { return m_queue.at(type); }
    Instance* getInstance() const { return m_pInstance; }

public:
    RenderGraph* getGraph() { return m_frameGraph[m_frameIdx].get(); }

public:
    UI* pUI = {};

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance* m_pInstance = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    std::unordered_map<QueueType, Queue*> m_queue;

protected:
    std::vector<std::unique_ptr<RenderGraph>> m_frameGraph;
    uint32_t                                  m_frameIdx = {};
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
