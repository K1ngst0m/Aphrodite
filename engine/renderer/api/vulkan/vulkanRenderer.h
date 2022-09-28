#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "common.h"

#include "renderer/renderer.h"

#include "device.h"
#include "pipeline.h"
#include "sceneRenderer.h"
#include "swapChain.h"
#include "texture.h"

namespace vkl {
struct PerFrameSyncObject {
    VkSemaphore renderSemaphore;
    VkSemaphore presentSemaphore;
    VkFence     inFlightFence;
    void        destroy(VkDevice device) const {
        vkDestroySemaphore(device, renderSemaphore, nullptr);
        vkDestroySemaphore(device, presentSemaphore, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);
    }
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
    void                           initDefaultResource();
    std::shared_ptr<SceneRenderer> createSceneRenderer() override;
    VkQueue                        getDeviceQueue(DeviceQueueType type) const;
    VkRenderPass                   getDefaultRenderPass() const;
    uint32_t                       getCommandBufferCount() const;
    VkCommandBuffer                getDefaultCommandBuffers(uint32_t idx) const;
    PipelineBuilder               &getPipelineBuilder();
    VkRenderPassBeginInfo          getDefaultRenderPassBeginInfo(uint32_t imageIdx);
    VkRenderPass                   createRenderPass(const std::vector<VkAttachmentDescription> &colorAttachments, VkAttachmentDescription &depthAttachment);

    std::shared_ptr<VulkanDevice> getDevice();
    void                          prepareUI();
    void                          initImGui();

    void recordSinglePassCommandBuffer(VkRenderPass renderPass, const std::function<void()> &drawCommands, uint32_t commandIdx);
    void recordCommandBuffer(const std::function<void()> &commands, uint32_t commandIdx);

private:
    void _createInstance();
    void _createDevice();
    void _createSurface();
    void _setupDebugMessenger();
    void _setupSwapChain();
    void _recreateSwapChain();
    void _createSwapChainImageViews();
    bool _checkValidationLayerSupport();

private:
    void _createDefaultDepthResources();
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
    VkInstance                      m_instance;
    std::shared_ptr<VulkanDevice>   m_device;
    VulkanSwapChain                 m_swapChain;
    std::vector<const char *>       m_supportedInstanceExtensions;
    VkPhysicalDeviceFeatures        m_enabledFeatures{};
    VkDebugUtilsMessengerEXT        m_debugMessenger;
    VkSurfaceKHR                    m_surface;
    PipelineBuilder                 m_pipelineBuilder;
    DeletionQueue                   m_deletionQueue;

private:
    VkRenderPass                    m_defaultRenderPass;
    std::vector<VkCommandBuffer>    m_defaultCommandBuffers;
    std::vector<PerFrameSyncObject> m_defaultSyncObjects;
    std::vector<VkFramebuffer>      m_defaultFramebuffers;
    vkl::VulkanTexture              m_defaultDepthAttachment;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIdx     = 0;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
