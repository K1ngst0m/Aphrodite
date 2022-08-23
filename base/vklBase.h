#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include "camera.h"
#include "vklUtils.h"
#include "vklInit.hpp"
#include "vklDevice.h"
#include "vklMesh.h"
#include "vklModel.h"

namespace vkl
{

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct WindowData {
    uint32_t width;
    uint32_t height;
    WindowData(uint32_t w, uint32_t h)
            : width(w)
            , height(h)
    {
    }
};

struct FrameData {
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};

struct MouseData {
    float lastX;
    float lastY;
    bool firstMouse = true;
    bool isCursorDisable = false;
    MouseData(float lastXin, float lastYin)
            : lastX(lastXin)
            , lastY(lastYin)
    {
    }
};

class vklBase {
public:
    vklBase();

    virtual ~vklBase() = default;

public:
    void init();
    void run();
    void finish();

protected:
    const std::filesystem::path assetDir = "data";
    const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
    const std::filesystem::path textureDir = assetDir / "textures";
    const std::filesystem::path modelDir = assetDir / "models";

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
    void prepareFrame();
    void submitFrame();

protected:
    void initWindow();
    void initVulkan();
    void cleanup();

protected:
    virtual void initDerive() {}
    virtual void cleanupDerive() {}
    virtual void keyboardHandleDerive();
    virtual void mouseHandleDerive(int xposIn, int yposIn);
    virtual void createSyncObjects();

    virtual void getEnabledFeatures() {}
    virtual void drawFrame() {}

    virtual void createCommandBuffers();

protected:
    std::vector<const char *> getRequiredInstanceExtensions();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    void loadImageFromFile(vkl::Texture &texture, std::string_view imagePath);

protected:
    Device *m_device;

protected:
    const std::string m_sessionName;

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

    vkl::Texture m_depthAttachment;

    std::vector<VkFramebuffer> m_Framebuffers;

    VkRenderPass m_renderPass;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    VkDescriptorPool m_descriptorPool;

    std::vector<VkCommandBuffer> m_commandBuffers;

    bool m_framebufferResized = false;

    uint32_t m_currentFrame = 0;
    std::vector<uint32_t> m_imageIndices;

    WindowData m_windowData;
    FrameData m_frameData;
    MouseData m_mouseData;
    Camera m_camera;
};
}

#endif // VULKANBASE_H_
