#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include <vector>
#include <optional>
#include <array>
#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "vklUtils.h"
#include "vklInit.hpp"
#include "vklBuffer.h"
#include "vklDevice.h"

namespace vkl
{

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class vkBase {
public:
    virtual ~vkBase() = default;

public:
    void init()
    {
        initWindow();
        initVulkan();
        initDerive();
    }

    void run()
    {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            keyboardHandleDerive();
            drawFrame();
        }

        vkDeviceWaitIdle(m_device->logicalDevice);
    }

    void finish()
    {
        cleanupDerive();
        cleanup();
    }

protected:
    const std::filesystem::path assetDir = "data";
    const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
    const std::filesystem::path textureDir = assetDir / "textures";

protected:
    struct Texture {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;
        VkSampler sampler;
        VkDescriptorImageInfo descriptorInfo;

        void cleanup(VkDevice device) const
        {
            vkDestroySampler(device, sampler, nullptr);
            vkDestroyImageView(device, imageView, nullptr);
            vkDestroyImage(device, image, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
    };

protected:
    struct {
        bool isEnableValidationLayer = true;
        const uint32_t max_frames = 2;
    } m_settings;

protected:
    void createDevice();
    void createInstance();
    bool checkValidationLayerSupport();
    void createSwapChain();
    void createSurface();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createDepthResources();
    void createRenderPass();
    void createSwapChainImageViews();
    void createFramebuffers();

protected:
    void initWindow();
    void initVulkan();
    void cleanup();

protected:
    virtual void initDerive() {}
    virtual void cleanupDerive() {}
    virtual void keyboardHandleDerive() {}
    virtual void mouseHandleDerive(int xposIn, int yposIn) {}

    virtual void getEnabledFeatures() {}
    virtual void drawFrame() {}

    virtual void createCommandBuffers();

protected:
    std::vector<const char *> getRequiredInstanceExtensions();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    void loadImageFromFile(VkImage &image, VkDeviceMemory &memory, std::string_view imagePath);

protected:
    Device *m_device;

protected:
    GLFWwindow *m_window = nullptr;
    VkInstance m_instance;
    std::vector<const char *> m_supportedInstanceExtensions;

    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapChain;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;

    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;

    std::vector<VkFramebuffer> m_Framebuffers;

    VkRenderPass m_renderPass;

    VkDescriptorPool m_descriptorPool;

    std::vector<VkCommandBuffer> m_commandBuffers;

    bool m_framebufferResized = false;

    uint32_t m_currentFrame = 0;

    Camera m_camera;

    uint32_t m_width = 1280;
    uint32_t m_height = 720;

    float deltaTime = 0.0f; // Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    float lastX = m_width / 2.0f;
    float lastY = m_height / 2.0f;
    bool firstMouse = true;
};
}

#endif // VULKANBASE_H_
