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
enum class DeviceQueueType {
    COMPUTE,
    GRAPHICS,
    TRANSFER,
    PRESENT,
};

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer()  = default;
    ~VulkanRenderer() = default;

    void initDevice() override;
    void destroyDevice() override;
    void idleDevice() override;

public:
    std::shared_ptr<SceneRenderer> createSceneRenderer() override;
    VkQueue                        getDeviceQueue(DeviceQueueType type) const;

public:
    void initDefaultResource();

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
    void _createDepthResources();
    void _createRenderPass();
    void _createFramebuffers();
    void _setupPipelineBuilder();
    void _createSyncObjects();
    void _createCommandBuffers();

public:
    void recordSinglePassCommandBuffer(WindowData *windowData, VkRenderPass renderPass, const std::function<void()> &drawCommands, uint32_t frameIdx);
    void recordSinglePassCommandBuffer(WindowData *windowData, const std::function<void()> &drawCommands, uint32_t frameIdx);

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkRenderPass         m_defaultRenderPass;
    vkl::PipelineBuilder m_pipelineBuilder;

private:
    void getEnabledFeatures() {
    }

private:
    std::vector<const char *> getRequiredInstanceExtensions();
    void                      immediateSubmit(VkQueue queue, std::function<void(VkCommandBuffer cmd)> &&function) const;
    void                      destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                            const VkAllocationCallbacks *pAllocator);

    vkl::DeletionQueue m_deletionQueue;

private:
    struct {
        VkQueue graphics;
        VkQueue present;
        VkQueue transfer;
        VkQueue compute;
    } m_queues;

    VkInstance                m_instance;
    std::vector<const char *> m_supportedInstanceExtensions;
    VkDebugUtilsMessengerEXT  m_debugMessenger;

    VkSurfaceKHR m_surface;

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

    std::vector<PerFrameSyncObject> m_frameSyncObjects;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIdx     = 0;

public:
    VulkanDevice            *m_device;
    VkPhysicalDeviceFeatures enabledFeatures{};

public:
    void prepareFrame() override;
    void submitFrame() override;

    bool m_framebufferResized = false;

    void prepareUI();
    void initImGui();

    VulkanSwapChain m_swapChain;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
