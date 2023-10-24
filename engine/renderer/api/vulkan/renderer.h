#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
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

    void beginFrame() override;
    void endFrame() override;

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

    VkSemaphore getRenderSemaphore() { return m_renderSemaphore[m_frameIdx]; }
    VkFence     getFrameFence() { return m_frameFence[m_frameIdx]; }

    CommandBuffer* acquireCommandBuffer(Queue* queue);
    VkSemaphore    acquireSemahpore();
    VkFence        acquireFence();
    Instance*      getInstance() const { return m_pInstance; }

public:
    UI * pUI = {};

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance* m_pInstance = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    std::unordered_map<QueueType, Queue*> m_queue;

protected:
    std::unique_ptr<SyncPrimitivesPool> m_pSyncPrimitivesPool = {};
    std::vector<VkSemaphore>            m_timelineMain        = {};
    std::vector<VkSemaphore>            m_renderSemaphore     = {};
    std::vector<VkFence>                m_frameFence          = {};

protected:
    struct FrameData
    {
        std::vector<CommandBuffer*> cmds;
        std::vector<VkSemaphore>    semaphores;
        std::vector<VkFence>        fences;
    };
    FrameData m_frameData;

protected:
    uint32_t m_frameIdx     = {};
    float    m_frameTimer   = {};
    uint32_t m_lastFPS      = {};
    uint32_t m_frameCounter = {};

    std::chrono::time_point<std::chrono::high_resolution_clock> m_timer = {};
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp, m_tStart, m_tPrevEnd;
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
