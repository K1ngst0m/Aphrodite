#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "device.h"
#include "renderer/renderer.h"
#include "shader.h"

namespace vkl {
class VulkanRenderer : public Renderer {
public:
    static std::unique_ptr<VulkanRenderer> Create(RenderConfig *config, std::shared_ptr<WindowData> windowData);
    VulkanRenderer(std::shared_ptr<WindowData> windowData, RenderConfig *config);

    ~VulkanRenderer() = default;

    void cleanup() override;
    void idleDevice() override;
    void prepareFrame();
    void submitFrame();

public:
    void _initDefaultResource();

public:
    VulkanInstance      *getInstance() const;
    VulkanDevice        *getDevice() const;
    VkQueue              getDefaultDeviceQueue(VkQueueFlags flags) const;
    VulkanRenderPass    *getDefaultRenderPass() const;
    uint32_t             getCommandBufferCount() const;
    VulkanCommandBuffer *getDefaultCommandBuffer(uint32_t idx) const;
    VulkanFramebuffer   *getDefaultFrameBuffer(uint32_t idx) const;
    VkPipelineCache      getPipelineCache();
    uint32_t             getCurrentFrameIndex() const;
    uint32_t             getCurrentImageIndex() const;
    VkExtent2D           getSwapChainExtent() const;

private:
    void _createInstance();
    void _createDevice();
    void _createSurface();
    void _setupDebugMessenger();
    void _setupSwapChain();

private:
    void _createDefaultDepthAttachments();
    void _createDefaultColorAttachments();
    void _createDefaultRenderPass();
    void _createDefaultFramebuffers();
    void _createDefaultSyncObjects();
    void _createPipelineCache();
    void _allocateDefaultCommandBuffers();

private:
    // TODO
    void getEnabledFeatures() {
    }

private:
    std::vector<const char *> getRequiredInstanceExtensions();

private:
    VulkanInstance          *m_instance;
    VulkanDevice            *m_device;
    VulkanSwapChain         *m_swapChain;
    VkPhysicalDeviceFeatures m_enabledFeatures{};
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR             m_surface;

    VkPipelineCache   m_pipelineCache;
    DeletionQueue     m_deletionQueue;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIdx     = 0;

    // default resource
private:
    vkl::VulkanRenderPass *m_renderPass;

    struct {
        std::vector<VulkanFramebuffer *> framebuffer;
        std::vector<VulkanImage *>       colorImage;
        std::vector<VulkanImageView *>   colorImageView;

        // TODO frames in flight depth attachment
        VulkanImage     *depthImage;
        VulkanImageView *depthImageView;
    } m_framebufferData;

    std::vector<VkSemaphore> m_renderSemaphore;
    std::vector<VkSemaphore> m_presentSemaphore;
    std::vector<VkFence>     m_inFlightFence;

    std::vector<VulkanCommandBuffer *> m_commandBuffers;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
