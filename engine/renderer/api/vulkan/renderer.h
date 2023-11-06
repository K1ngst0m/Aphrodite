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
    void    submit(Queue* pQueue, QueueSubmitInfo submitInfos, Image* pPresentImage = nullptr);
    Shader* getShaders(const std::filesystem::path& path) const;
    Queue*  getDefaultQueue(QueueType type) const { return m_queue.at(type); }

    Semaphore* getRenderSemaphore() { return m_frameData[m_frameIdx].renderSemaphore; }
    Fence*     getFrameFence() { return m_frameData[m_frameIdx].fence; }

    Semaphore*   acquireSemahpore();
    Fence*       acquireFence();
    Instance*    getInstance() const { return m_pInstance; }

    VkQueryPool getFrameQueryPool() const { return m_frameData[m_frameIdx].queryPool; }

public:
    RenderGraph* getGraph()
    {
        if(!m_frameData[m_frameIdx].pGraph)
        {
            m_frameData[m_frameIdx].pGraph = std::make_unique<RenderGraph>(m_pDevice.get());
        }
        return m_frameData[m_frameIdx].pGraph.get();
    }

public:
    UI* pUI = {};

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance* m_pInstance = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    std::unordered_map<QueueType, Queue*> m_queue;

protected:
    struct FrameData
    {
        Semaphore*  renderSemaphore = {};
        Fence*      fence           = {};
        VkQueryPool queryPool       = {};

        std::unique_ptr<RenderGraph> pGraph;
        std::vector<CommandPool*>    cmdPools;
        std::vector<Semaphore*>      semaphores;
        std::vector<Fence*>          fences;
    };
    std::vector<FrameData> m_frameData;

    void resetFrameData();

protected:
    uint32_t m_frameIdx       = {};
    double   m_frameTime      = {};
    double   m_framePerSecond = {};
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
