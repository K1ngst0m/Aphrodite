#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "common.h"

#include "renderer/renderer.h"

#include "device.h"
#include "pipeline.h"
#include "sceneRenderer.h"
#include "swapChain.h"

namespace vkl {
struct PerFrameSyncObject {
    VkSemaphore renderSemaphore;
    VkSemaphore presentSemaphore;
    VkFence     inFlightFence;
};

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer()  = default;
    ~VulkanRenderer() = default;

    void initDevice() override;
    void destroyDevice() override;
    void idleDevice() override;
    void prepareFrame() override;
    void submitFrame() override;

public:
    void initDefaultResource();
    void initImGui();
    void prepareUIDraw();
    void recordCommandBuffer(const std::function<void()> &commands, uint32_t commandIdx);

public:
    VkQueue          getDeviceQueue(DeviceQueueType type) const;
    VkRenderPass     getDefaultRenderPass() const;
    uint32_t         getCommandBufferCount() const;
    VkCommandBuffer  getDefaultCommandBuffer(uint32_t idx) const;
    VkFramebuffer    getDefaultFrameBuffer(uint32_t idx) const;
    PipelineBuilder &getPipelineBuilder();

    std::shared_ptr<VulkanDevice>  getDevice();
    std::shared_ptr<SceneRenderer> getSceneRenderer() override;

private:
    void _createInstance();
    void _createDevice();
    void _createSurface();
    void _setupDebugMessenger();
    void _setupSwapChain();
    void _recreateSwapChain();
    bool _checkValidationLayerSupport();

private:
    void _createDefaultDepthAttachments();
    void _createDefaultColorAttachments();
    void _createDefaultRenderPass();
    void _createDefaultFramebuffers();
    void _setupPipelineBuilder();
    void _createSyncObjects();
    void _createCommandBuffers();

private:
    // TODO
    void getEnabledFeatures() {
    }

private:
    std::vector<const char *> getRequiredInstanceExtensions();
    void                      immediateSubmit(VkQueue queue, std::function<void(VkCommandBuffer cmd)> &&function) const;
    void                      destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                            const VkAllocationCallbacks *pAllocator);

private:
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VkQueue computeQueue;

private:
    VkInstance                    m_instance;
    std::shared_ptr<VulkanDevice> m_device;
    VulkanSwapChain               m_swapChain;
    std::vector<const char *>     m_supportedInstanceExtensions;
    VkPhysicalDeviceFeatures      m_enabledFeatures{};
    VkDebugUtilsMessengerEXT      m_debugMessenger;
    VkSurfaceKHR                  m_surface;
    PipelineBuilder               m_pipelineBuilder;
    DeletionQueue                 m_deletionQueue;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIdx     = 0;

private:
    std::vector<VkCommandBuffer>    m_defaultCommandBuffers;
    std::vector<PerFrameSyncObject> m_defaultSyncObjects;

    struct FrameBufferData {
        VulkanImage       *colorImage;
        VulkanImageView   *colorImageView;
        VulkanImage       *depthImage;
        VulkanImageView   *depthImageView;
        VulkanFramebuffer *framebuffer;
    };
    std::vector<FrameBufferData> m_defaultFramebuffers;
    VulkanRenderPass            *m_defaultRenderPass;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
