#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "api/vulkan/shader.h"
#include "renderer/renderer.h"

namespace aph
{
class VulkanRenderer : public IRenderer
{
public:
    VulkanRenderer(std::shared_ptr<Window> window, const RenderConfig& config);
    ~VulkanRenderer();

    void beginFrame();
    void endFrame();

public:
    VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
    VulkanInstance*       getInstance() const { return m_pInstance; }
    VulkanDevice*         getDevice() const { return m_pDevice; }
    VkPipelineCache       getPipelineCache() { return m_pipelineCache; }
    VulkanSwapChain*      getSwapChain() { return m_pSwapChain; }
    VulkanShaderModule*   getShaders(const std::filesystem::path& path);

    VulkanSyncPrimitivesPool* getSyncPrimitiviesPool() { return m_pSyncPrimitivesPool.get(); }
    uint32_t                  getCommandBufferCount() const { return m_commandBuffers.size(); }

    VulkanQueue* getGraphicsQueue() const { return m_queue.graphics; }
    VulkanQueue* getComputeQueue() const { return m_queue.compute; }
    VulkanQueue* getTransferQueue() const { return m_queue.transfer; }

protected:
    std::unique_ptr<VulkanSyncPrimitivesPool> m_pSyncPrimitivesPool = {};

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_8_BIT};

    VulkanInstance*  m_pInstance  = {};
    VulkanDevice*    m_pDevice    = {};
    VulkanSwapChain* m_pSwapChain = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    struct
    {
        VulkanQueue* graphics = {};
        VulkanQueue* compute  = {};
        VulkanQueue* transfer = {};
    } m_queue;

protected:
    std::vector<VkSemaphore> m_renderSemaphore  = {};
    std::vector<VkSemaphore> m_presentSemaphore = {};
    std::vector<VkFence>     m_frameFences      = {};

    std::vector<VulkanCommandBuffer*> m_commandBuffers = {};

protected:
    uint32_t m_frameIdx = {};
    uint32_t m_imageIdx = {};

protected:
    float    m_frameTimer   = {};
    uint32_t m_lastFPS      = {};
    uint32_t m_frameCounter = {};

    std::chrono::time_point<std::chrono::high_resolution_clock> m_timer = {};
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp, m_tStart, m_tPrevEnd;

    std::unordered_map<std::string, VulkanShaderModule*> shaderModuleCaches = {};

protected:
    bool updateUIDrawData(float deltaTime);
    void recordUIDraw(VulkanCommandBuffer* pCommandBuffer);
    struct UI
    {
        void resize(uint32_t width, uint32_t height);
        struct PushConstBlock
        {
            glm::vec2 scale;
            glm::vec2 translate;
        } m_pushConstBlock;

        bool visible = {true};
        bool updated = {false};

        VulkanImage*     m_pFontImage  = {};
        VkSampler        m_fontSampler = {};
        VkDescriptorPool m_pool        = {};
        VkRenderPass     m_renderpass  = {};
        VulkanPipeline*  m_pPipeline   = {};

        VulkanBuffer* m_pVertexBuffer = {};
        VulkanBuffer* m_pIndexBuffer  = {};
        uint32_t      m_vertexCount   = {};
        uint32_t      m_indexCount    = {};

        VulkanDescriptorSetLayout* m_pSetLayout = {};
        VkDescriptorSet            m_set        = {};

        float m_scale = {1.1f};
    } m_ui;
};
}  // namespace aph

#endif  // VULKANRENDERER_H_
