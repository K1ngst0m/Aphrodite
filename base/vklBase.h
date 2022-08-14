#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
#include "vklInit.h"

namespace vkl
{
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
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

        vkDeviceWaitIdle(m_device);
    }

    void finish()
    {
        cleanupDerive();
        cleanup();
    }

protected:
    const static std::filesystem::path assetDir;
    const static std::filesystem::path glslShaderDir;
    const static std::filesystem::path textureDir;

protected:
    struct Texture {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;
        VkSampler sampler;
        VkDescriptorImageInfo descriptorInfo;

        void cleanup(VkDevice device) const{
            vkDestroySampler(device, sampler, nullptr);
            vkDestroyImageView(device, imageView, nullptr);
            vkDestroyImage(device, image, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
    };

    struct VertexBuffer{
        VkDeviceMemory memory;
        VkBuffer buffer;

        void cleanup(VkDevice device) const{
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
    };

    struct IndexBuffer{
        VkDeviceMemory memory;
        VkBuffer buffer;
        uint32_t count;

        void cleanup(VkDevice device) const{
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
    };

    struct UniformBuffer{
        VkDeviceMemory memory;
        VkBuffer buffer;
        VkDescriptorBufferInfo descriptorInfo;

        void cleanup(VkDevice device) const{
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
    };
protected:
    struct {
        bool isEnableValidationLayer = true;
        const uint32_t max_frames = 2;
    } m_settings;

protected:
    void initWindow();
    void initVulkan();
    void cleanup();

protected:
    virtual void initDerive();
    virtual void cleanupDerive();
    virtual void keyboardHandleDerive();
    virtual void mouseHandleDerive(int xposIn, int yposIn);

protected:
    virtual void createInstance();
    virtual bool checkValidationLayerSupport();
    virtual void getEnabledFeatures();
    virtual void createSwapChain();
    virtual void createSurface();
    virtual void recreateSwapChain();
    virtual void cleanupSwapChain();
    virtual VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    virtual VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    virtual VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    virtual void createSwapChainImageViews();

    virtual void pickPhysicalDevice();
    virtual bool isDeviceSuitable(VkPhysicalDevice device);

    virtual void createLogicalDevice();
    virtual bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    virtual void createRenderPass();
    virtual void createDepthResources();

    virtual void createFramebuffers();
    virtual void createCommandPool();
    virtual void createCommandBuffers();

    virtual void drawFrame();

protected:
    std::vector<const char *> getRequiredExtensions();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    VkShaderModule createShaderModule(const std::vector<char> &code);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkImageView createImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    void loadImageFromFile(VkImage &image, VkDeviceMemory &memory, std::string_view imagePath);

protected:
    GLFWwindow *m_window = nullptr;
    VkInstance m_instance;
    std::vector<const char *> m_supportedInstanceExtensions;

    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
    VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties;

    VkDevice m_device;

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

    VkCommandPool m_commandPool;
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
