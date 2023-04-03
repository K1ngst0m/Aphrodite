#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "renderer/api/vulkan/device.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/renderer.h"

namespace vkl
{
class VulkanRenderer : public Renderer
{
public:
    VulkanRenderer(std::shared_ptr<WindowData> windowData, const RenderConfig &config);

    ~VulkanRenderer() = default;

    void cleanup() override;
    void idleDevice() override;
    void beginFrame();
    void endFrame();

public:
    VulkanInstance *getInstance() const { return m_instance; }
    VulkanDevice *getDevice() const { return m_device; }
    uint32_t getCurrentFrameIndex() const { return m_currentFrameIdx; }
    uint32_t getCurrentImageIndex() const { return m_imageIdx; }
    VkPipelineCache getPipelineCache() { return m_pipelineCache; }
    VulkanSwapChain *getSwapChain() { return m_swapChain; }
    VulkanQueue *getGraphicsQueue() const { return m_queue.graphics; }
    VulkanQueue *getComputeQueue() const { return m_queue.compute; }
    VulkanQueue *getTransferQueue() const { return m_queue.transfer; }

    VulkanSyncPrimitivesPool *getSyncPrimitiviesPool() { return m_pSyncPrimitivesPool; }
    VulkanShaderCache *getShaderCache() { return m_pShaderCache; }

private:
    VulkanSyncPrimitivesPool *m_pSyncPrimitivesPool = nullptr;
    VulkanShaderCache *m_pShaderCache = nullptr;

private:
    VulkanInstance *m_instance = nullptr;
    VulkanDevice *m_device = nullptr;
    VulkanSwapChain *m_swapChain = nullptr;
    VkSurfaceKHR m_surface;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;

    uint32_t m_currentFrameIdx = 0;
    uint32_t m_imageIdx = 0;

    struct
    {
        VulkanQueue *graphics = nullptr;
        VulkanQueue *compute = graphics;
        VulkanQueue *transfer = compute;
    } m_queue;

    // default resource
public:
    VulkanRenderPass *getDefaultRenderPass() const { return m_renderPass; }
    VulkanFramebuffer *getDefaultFrameBuffer(uint32_t idx) const { return m_defaultFb.framebuffers[idx]; }
    VulkanCommandBuffer *getDefaultCommandBuffer(uint32_t idx) const { return m_commandBuffers[idx]; }
    uint32_t getCommandBufferCount() const { return m_commandBuffers.size(); }

private:
    vkl::VulkanRenderPass *m_renderPass = nullptr;

    struct
    {
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VulkanImage *> colorImages;
        std::vector<VulkanImageView *> colorImageViews;
        std::vector<VulkanImage *> depthImages;
        std::vector<VulkanImageView *> depthImageViews;
    } m_defaultFb;

    std::vector<VkSemaphore> m_renderSemaphore;
    std::vector<VkSemaphore> m_presentSemaphore;
    std::vector<VkFence> m_frameFences;

    std::vector<VulkanCommandBuffer *> m_commandBuffers;
};
}  // namespace vkl

#endif  // VULKANRENDERER_H_
