#ifndef VULKANRENDERER_H_
#define VULKANRENDERER_H_

#include "common.h"

#include "renderer/renderer.h"

#include <vulkan/vulkan.h>

#include "device.h"
#include "pipeline.h"
#include "texture.h"

namespace vkl {
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct WindowData {
    uint32_t width;
    uint32_t height;
    WindowData(uint32_t w, uint32_t h)
        : width(w), height(h) {
    }
};

class VulkanRenderer : public Renderer {
    struct {
        bool           enableValidationLayers = true;
        bool           enableUI               = false;
        const uint32_t max_frames             = 2;
    } m_settings;

public:
    void init() override {
        createInstance();
        setupDebugMessenger();
        createSurface();
        createDevice();
        createSwapChain();
        createSwapChainImageViews();
        createCommandBuffers();
        createDepthResources();
        createRenderPass();
        createFramebuffers();
        setupPipelineBuilder();
        createSyncObjects();
    }
    void destroy() override {
        cleanupSwapChain();
        m_deletionQueue.flush();
    }

    void setWindow(GLFWwindow *window) {
        m_window = window;
    }

    void waitIdle() const {
        vkDeviceWaitIdle(m_device->logicalDevice);
    }

public:
    void createInstance();
    void createDevice();
    void createSurface();
    void setupDebugMessenger();
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createSwapChainImageViews();
    bool checkValidationLayerSupport();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void setupPipelineBuilder();

    virtual void createSyncObjects();
    virtual void createCommandBuffers();
    virtual void getEnabledFeatures() {
    }

    void recordCommandBuffer(WindowData* windowData, VkRenderPass renderPass, const std::function<void(VkCommandBuffer cmdBuffer)> &drawCommands, uint32_t frameIdx);
    void recordCommandBuffer(WindowData * windowData, const std::function<void(VkCommandBuffer cmdBuffer)> &drawCommands, uint32_t frameIdx);

public:
    std::vector<const char *> getRequiredInstanceExtensions();
    SwapChainSupportDetails   querySwapChainSupport(VkPhysicalDevice device);
    void                      immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);
    void                      loadImageFromFile(VulkanTexture &texture, std::string_view imagePath);
    void                      destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                            const VkAllocationCallbacks *pAllocator);

    vkl::DeletionQueue m_deletionQueue;

public:
    VkInstance                m_instance;
    std::vector<const char *> m_supportedInstanceExtensions;
    VkDebugUtilsMessengerEXT  m_debugMessenger;

    struct {
        VkQueue graphics;
        VkQueue present;
        VkQueue transfer;
    } m_queues;

    VkSurfaceKHR   m_surface;
    VkSwapchainKHR m_swapChain;
    VkFormat       m_swapChainImageFormat;
    VkExtent2D     m_swapChainExtent;

    std::vector<VkImage>       m_swapChainImages;
    std::vector<VkImageView>   m_swapChainImageViews;
    std::vector<VkFramebuffer> m_framebuffers;

    vkl::VulkanTexture m_depthAttachment;

    VkRenderPass m_defaultRenderPass;

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

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkDescriptorPool m_descriptorPool;

    vkl::PipelineBuilder m_pipelineBuilder;

    uint32_t m_currentFrame = 0;
public:
    uint32_t m_imageIdx;

public:
    VulkanDevice            *m_device;
    VkPhysicalDeviceFeatures enabledFeatures{};

    GLFWwindow *m_window = nullptr;

public:
    void prepareFrame();
    void submitFrame();

    bool m_framebufferResized = false;
};
} // namespace vkl

#endif // VULKANRENDERER_H_
