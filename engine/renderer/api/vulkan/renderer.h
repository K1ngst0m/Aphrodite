#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "api/vulkan/shader.h"
#include "renderer/renderer.h"

namespace aph::vk
{
class Renderer : public IRenderer
{
public:
    Renderer(WSI* wsi, const RenderConfig& config);
    ~Renderer();

    void beginFrame() override;
    void endFrame() override;

public:
    SwapChain* getSwapChain() const { return m_pSwapChain; }
    Device*    getDevice() const { return m_pDevice; }
    Shader*    getShaders(const std::filesystem::path& path);

    Queue* getGraphicsQueue() const { return m_queue.graphics; }
    Queue* getComputeQueue() const { return m_queue.compute; }
    Queue* getTransferQueue() const { return m_queue.transfer; }

    VkSemaphore getRenderSemaphore() { return m_renderSemaphore[m_frameIdx]; }

    VkSemaphore getPresentSemaphore() { return m_presentSemaphore[m_frameIdx]; }

    VkSemaphore acquireTimelineMain()
    {
        m_pSyncPrimitivesPool->acquireTimelineSemaphore(1, &m_timelineMain[m_frameIdx]);
        return m_timelineMain[m_frameIdx];
    }

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance*  m_pInstance  = {};
    Device*    m_pDevice    = {};
    SwapChain* m_pSwapChain = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    struct
    {
        Queue* graphics = {};
        Queue* compute  = {};
        Queue* transfer = {};
    } m_queue;

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaderModuleCaches = {};

protected:
    std::unique_ptr<SyncPrimitivesPool> m_pSyncPrimitivesPool = {};
    std::vector<VkSemaphore>            m_timelineMain        = {};
    std::vector<VkSemaphore>            m_renderSemaphore     = {};
    std::vector<VkSemaphore>            m_presentSemaphore    = {};

protected:
    uint32_t m_frameIdx     = {};
    float    m_frameTimer   = {};
    uint32_t m_lastFPS      = {};
    uint32_t m_frameCounter = {};

    std::chrono::time_point<std::chrono::high_resolution_clock> m_timer = {};
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp, m_tStart, m_tPrevEnd;

protected:
    bool onUIMouseMove(const MouseMoveEvent& e);
    bool onUIMouseBtn(const MouseButtonEvent& e);
    bool updateUIDrawData(float deltaTime);
    void recordUIDraw(CommandBuffer* pCommandBuffer);
    struct UI
    {
        void resize(uint32_t width, uint32_t height);
        struct PushConstBlock
        {
            glm::vec2 scale;
            glm::vec2 translate;
        } pushConstBlock;

        bool visible = {true};
        bool updated = {false};

        Image*           pFontImage  = {};
        Sampler*         fontSampler = {};
        VkDescriptorPool pool        = {};
        Pipeline*        pipeline    = {};
        ShaderProgram*   pProgram    = {};

        Buffer*  pVertexBuffer = {};
        Buffer*  pIndexBuffer  = {};
        uint32_t vertexCount   = {};
        uint32_t indexCount    = {};

        VkDescriptorSet set = {};

        float scale = {1.1f};
    } m_ui;
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
