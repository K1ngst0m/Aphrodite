#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "device.h"
#include "renderer/renderer.h"
#include "shader.h"

namespace vkl {
struct PerFrameSyncObject {
    VkSemaphore renderSemaphore;
    VkSemaphore presentSemaphore;
    VkFence     inFlightFence;
    uint32_t    imageIdx = 0;
};

class VulkanRenderer : public Renderer {
public:
    static std::unique_ptr<VulkanRenderer> Create(RenderConfig *config, std::shared_ptr<WindowData> windowData);
    VulkanRenderer(std::shared_ptr<WindowData> windowData, RenderConfig *config);

    ~VulkanRenderer() = default;

    void cleanup() override;
    void idleDevice() override;
    void drawDemo() override;
    void renderOneFrame() override;
    void prepareFrame();
    void submitFrame();

public:
    void _initDefaultResource();

public:
    VulkanInstance      *getInstance() const;
    VulkanDevice        *getDevice() const;
    VkQueue              getDefaultDeviceQueue(QueueFamilyType type) const;
    VulkanRenderPass    *getDefaultRenderPass() const;
    uint32_t             getCommandBufferCount() const;
    VulkanCommandBuffer *getDefaultCommandBuffer(uint32_t idx) const;
    VulkanFramebuffer   *getDefaultFrameBuffer(uint32_t idx) const;
    VulkanShaderCache   &getShaderCache();
    VkPipelineCache      getPipelineCache();
    uint32_t             getCurrentFrameIndex() const;
    uint32_t getCurrentFrameImageIndex() const{
        return m_defaultResource.syncObjects[m_currentFrame].imageIdx;
    }

private:

    void _createInstance();
    void _createDevice();
    void _createSurface();
    void _setupDebugMessenger();
    void _setupSwapChain();
    void _setupDemoPass();

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
    VkExtent2D           getSwapChainExtent() const;
    std::vector<const char *> getRequiredInstanceExtensions();
    void                      destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
    PerFrameSyncObject       &getCurrentFrameSyncObject();

private:
    VulkanInstance           *m_instance;
    VulkanDevice             *m_device;
    VulkanSwapChain          *m_swapChain;
    std::vector<const char *> m_supportedInstanceExtensions;
    VkPhysicalDeviceFeatures  m_enabledFeatures{};
    VkDebugUtilsMessengerEXT  m_debugMessenger;
    VkSurfaceKHR              m_surface;

    VkPipelineCache m_pipelineCache;
    DeletionQueue   m_deletionQueue;

    uint32_t m_currentFrame = 0;

private:
    struct FrameBufferData {
        VulkanImage       *colorImage;
        VulkanImageView   *colorImageView;
        VulkanImage       *depthImage;
        VulkanImageView   *depthImageView;
        VulkanFramebuffer *framebuffer;
    };

    struct {
        std::vector<VulkanCommandBuffer *> commandBuffers;
        std::vector<PerFrameSyncObject>    syncObjects;
        std::vector<FrameBufferData>       framebuffers;
        VulkanRenderPass                  *renderPass;
        VulkanPipeline                    *demoPipeline;
    } m_defaultResource;

    VulkanShaderCache m_shaderCache;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
