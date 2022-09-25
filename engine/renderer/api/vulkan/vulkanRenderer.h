#ifndef VULKANRENDERER_H_
#define VULKANRENDERER_H_

#include "common.h"

#include "renderer/renderer.h"

#include "device.h"
#include "pipeline.h"
#include "sceneRenderer.h"
#include "swapChain.h"
#include "texture.h"

namespace vkl {
struct WindowData {
    GLFWwindow *window = nullptr;
    uint32_t    width;
    uint32_t    height;
    WindowData(uint32_t w, uint32_t h)
        : width(w), height(h) {
    }
};

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer(WindowData *windowData);
    ~VulkanRenderer() = default;

    void initDevice() override;
    void destroyDevice() override;
    void idleDevice() override;
    void setWindow(void *window) override;

    std::shared_ptr<SceneRenderer> createSceneRenderer() override;

private:
    void _createInstance();
    void _createDevice();
    void _createSurface();
    void _setupDebugMessenger();
    void _setupSwapChain();
    void _recreateSwapChain();
    void _createSwapChainImageViews();
    bool _checkValidationLayerSupport();

public:
    void initDefaultResource();

private:
    void _createDepthResources();
    void _createRenderPass();
    void _createFramebuffers();
    void _setupPipelineBuilder();
    void _createSyncObjects();
    void _createCommandBuffers();

public:
    void recordCommandBuffer(WindowData *windowData, VkRenderPass renderPass, const std::function<void()> &drawCommands, uint32_t frameIdx);
    void recordCommandBuffer(WindowData *windowData, const std::function<void()> &drawCommands, uint32_t frameIdx);

    struct {
        VkQueue graphics;
        VkQueue present;
        VkQueue transfer;
    } m_queues;

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkRenderPass         m_defaultRenderPass;
    vkl::PipelineBuilder m_pipelineBuilder;

private:
    void getEnabledFeatures() {
    }

private:
    std::vector<const char *> getRequiredInstanceExtensions();
    void                      immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);
    void                      destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                            const VkAllocationCallbacks *pAllocator);

    vkl::DeletionQueue m_deletionQueue;

private:
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

    bool        m_framebufferResized = false;
    WindowData *m_windowData         = nullptr;

    void prepareUI();
    void initImGui();

    VulkanSwapChain m_swapChain;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
