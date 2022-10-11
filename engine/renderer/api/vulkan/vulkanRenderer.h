#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include <utility>

#include "instance.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/renderer.h"
#include "sceneRenderer.h"

namespace vkl {
struct PerFrameSyncObject {
    VkSemaphore renderSemaphore;
    VkSemaphore presentSemaphore;
    VkFence     inFlightFence;
    uint32_t    imageIdx = 0;
};

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer(std::shared_ptr<WindowData> windowData, RenderConfig *config);

    ~VulkanRenderer() = default;

    void destroy() override;
    void idleDevice() override;
    void drawDemo() override;
    void renderOneFrame() override;

public:
    void _initDefaultResource();

public:
    VulkanInstance      *getInstance() const;
    VulkanDevice        *getDevice() const;
    VkQueue              getDefaultDeviceQueue(QueueFlags type) const;
    VulkanRenderPass    *getDefaultRenderPass() const;
    uint32_t             getCommandBufferCount() const;
    VulkanCommandBuffer *getDefaultCommandBuffer(uint32_t idx) const;
    VulkanFramebuffer   *getDefaultFrameBuffer(uint32_t idx) const;
    PipelineBuilder     &getPipelineBuilder();
    ShaderCache         &getShaderCache();
    void                 resetPipelineBuilder();

    std::shared_ptr<SceneRenderer> getSceneRenderer() override;
    std::shared_ptr<UIRenderer>    getUIRenderer() override;

private:
    void prepareFrame();
    void submitFrame();

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
    void _allocateDefaultCommandBuffers();

private:
    // TODO
    void getEnabledFeatures() {
    }

private:
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
    PipelineBuilder           m_pipelineBuilder;
    DeletionQueue             m_deletionQueue;

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
        ShaderPass                        *demoPass;
        VulkanPipelineLayout              *demoLayout;
    } m_defaultResource;

    ShaderCache m_shaderCache;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
