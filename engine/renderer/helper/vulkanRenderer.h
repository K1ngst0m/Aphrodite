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
    void prepareFrame();
    void submitAndPresent();

public:
    VkPipelineCache getPipelineCache() { return m_pipelineCache; }
    uint32_t getCurrentFrameIndex() const { return m_currentFrame; }
    uint32_t getCurrentImageIndex() const { return m_imageIdx; }
    VulkanSwapChain *getSwapChain() { return m_swapChain; }
    VulkanRenderPass *getDefaultRenderPass() const { return m_renderPass; }
    VulkanCommandBuffer *getDefaultCommandBuffer(uint32_t idx) const { return m_commandBuffers[idx]; }
    uint32_t getCommandBufferCount() const { return m_commandBuffers.size(); }
    VulkanDevice *getDevice() const { return m_device; }
    VulkanFramebuffer *getDefaultFrameBuffer(uint32_t idx) const { return m_fbData.framebuffers[idx]; }
    VulkanInstance *getInstance() const { return m_instance; }
    VulkanQueue* getGraphicsQueue() const {return m_queue.graphics;}
    VulkanQueue* getComputeQueue() const {return m_queue.compute;}
    VulkanQueue* getTransferQueue() const {return m_queue.transfer;}

private:
    void _createDefaultRenderPass();
    void _createDefaultFramebuffers();
    void _createDefaultSyncObjects();
    void _createPipelineCache();
    void _allocateDefaultCommandBuffers();

private:
    VulkanInstance *m_instance = nullptr;
    VulkanDevice *m_device = nullptr;
    VulkanSwapChain *m_swapChain = nullptr;

    VkPhysicalDeviceFeatures m_enabledFeatures{};
    VkSurfaceKHR m_surface;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIdx = 0;

    // default resource
private:
    vkl::VulkanRenderPass *m_renderPass = nullptr;

    struct
    {
        VulkanQueue *graphics = nullptr;
        VulkanQueue *compute = graphics;
        VulkanQueue *transfer = compute;
    } m_queue;

    struct
    {
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VulkanImage *> colorImages;
        std::vector<VulkanImageView *> colorImageViews;

        // TODO frames in flight depth attachment
        VulkanImage *depthImage = nullptr;
        VulkanImageView *depthImageView = nullptr;
    } m_fbData;

    std::vector<VkSemaphore> m_renderSemaphore;
    std::vector<VkSemaphore> m_presentSemaphore;
    std::vector<VkFence> m_inFlightFence;

    std::vector<VulkanCommandBuffer *> m_commandBuffers;
};
}  // namespace vkl

#endif  // VULKANRENDERER_H_
