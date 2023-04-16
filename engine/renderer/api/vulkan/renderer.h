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
    VulkanInstance*  getInstance() const { return m_instance; }
    VulkanDevice*    getDevice() const { return m_device; }
    uint32_t         getCurrentFrameIndex() const { return m_frameIdx; }
    uint32_t         getCurrentImageIndex() const { return m_imageIdx; }
    VkPipelineCache  getPipelineCache() { return m_pipelineCache; }
    VulkanSwapChain* getSwapChain() { return m_swapChain; }

    VulkanSyncPrimitivesPool* getSyncPrimitiviesPool() { return m_pSyncPrimitivesPool; }
    VulkanShaderCache*        getShaderCache() { return m_pShaderCache; }
    VulkanCommandBuffer*      getDefaultCommandBuffer(uint32_t idx) const { return m_commandBuffers[idx]; }
    uint32_t                  getCommandBufferCount() const { return m_commandBuffers.size(); }
    VulkanQueue*              getGraphicsQueue() const { return m_queue.graphics; }
    VulkanQueue*              getComputeQueue() const { return m_queue.compute; }
    VulkanQueue*              getTransferQueue() const { return m_queue.transfer; }

private:
    VulkanSyncPrimitivesPool* m_pSyncPrimitivesPool{};
    VulkanShaderCache*        m_pShaderCache{};

private:
    VulkanInstance*  m_instance{};
    VulkanDevice*    m_device{};
    VulkanSwapChain* m_swapChain{};
    VkSurfaceKHR     m_surface{};

    VkPipelineCache m_pipelineCache{};

    uint32_t m_frameIdx = 0;
    uint32_t m_imageIdx = 0;

    struct
    {
        VulkanQueue* graphics{};
        VulkanQueue* compute{};
        VulkanQueue* transfer{};
    } m_queue;

private:
    std::vector<VkSemaphore> m_renderSemaphore;
    std::vector<VkSemaphore> m_presentSemaphore;
    std::vector<VkFence>     m_frameFences;

    std::vector<VulkanCommandBuffer*> m_commandBuffers;
};
}  // namespace aph

#endif  // VULKANRENDERER_H_
