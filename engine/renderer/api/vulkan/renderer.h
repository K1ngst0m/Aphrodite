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

    ~VulkanRenderer() = default;

    void cleanup() override;
    void idleDevice() override;
    void beginFrame();
    void endFrame();

public:
    VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
    RenderConfig          getConfig() const { return m_config; }
    VulkanInstance*       getInstance() const { return m_pInstance; }
    VulkanDevice*         getDevice() const { return m_pDevice; }
    uint32_t              getCurrentFrameIndex() const { return m_frameIdx; }
    uint32_t              getCurrentImageIndex() const { return m_imageIdx; }
    VkPipelineCache       getPipelineCache() { return m_pipelineCache; }
    VulkanSwapChain*      getSwapChain() { return m_pSwapChain; }
    VulkanShaderModule*   getShaders(const std::filesystem::path& path);

    VulkanSyncPrimitivesPool* getSyncPrimitiviesPool() { return m_pSyncPrimitivesPool; }
    VulkanCommandBuffer*      getDefaultCommandBuffer(uint32_t idx) const { return m_commandBuffers[idx]; }
    uint32_t                  getCommandBufferCount() const { return m_commandBuffers.size(); }

    VulkanQueue* getGraphicsQueue() const { return m_queue.graphics; }
    VulkanQueue* getComputeQueue() const { return m_queue.compute; }
    VulkanQueue* getTransferQueue() const { return m_queue.transfer; }

protected:
    VulkanSyncPrimitivesPool* m_pSyncPrimitivesPool = {};

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
    std::vector<VkSemaphore>          m_renderSemaphore  = {};
    std::vector<VkSemaphore>          m_presentSemaphore = {};
    std::vector<VkFence>              m_frameFences      = {};
    std::vector<VulkanCommandBuffer*> m_commandBuffers   = {};

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
};
}  // namespace aph

#endif  // VULKANRENDERER_H_
