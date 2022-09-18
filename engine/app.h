#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include "vkl.hpp"

namespace vkl {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct WindowData {
    uint32_t width;
    uint32_t height;
    WindowData(uint32_t w, uint32_t h) : width(w), height(h) {
    }
};

struct PerFrameData {
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};

struct MouseData {
    float lastX;
    float lastY;
    bool  firstMouse      = true;
    bool  isCursorDisable = false;
    MouseData(float lastXin, float lastYin) : lastX(lastXin), lastY(lastYin) {
    }
};

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()> &&function) {
        deletors.push_back(function);
    }

    void flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)();
        }

        deletors.clear();
    }
};

class vklApp {
public:
    vklApp(std::string sessionName = "", uint32_t winWidth = 800, uint32_t winHeight = 600);

    virtual ~vklApp() = default;

public:
    void init();
    void run();
    void finish();

protected:
    const std::filesystem::path assetDir      = "assets";
    const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
    const std::filesystem::path textureDir    = assetDir / "textures";
    const std::filesystem::path modelDir      = assetDir / "models";

protected:
    struct {
        bool           enableValidationLayers = true;
        bool           enableUI               = false;
        const uint32_t max_frames             = 2;
    } m_settings;

protected:
    void createDevice();
    void createInstance();
    bool checkValidationLayerSupport();
    void setupDebugMessenger();
    void createSwapChain();
    void createSurface();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createDepthResources();
    void createRenderPass();
    void createSwapChainImageViews();
    void createFramebuffers();
    void setupPipelineBuilder();
    void prepareFrame();
    void prepareUI();
    void submitFrame();

protected:
    void initWindow();
    void initVulkan();
    void initImGui();
    void cleanup();

protected:
    virtual void initDerive() {
    }
    virtual void cleanupDerive() {
    }
    virtual void keyboardHandleDerive();
    virtual void mouseHandleDerive(int xposIn, int yposIn);
    virtual void createSyncObjects();

    virtual void getEnabledFeatures() {
    }
    virtual void drawFrame() {
    }

    virtual void createCommandBuffers();
    void recordCommandBuffer(VkRenderPass renderPass, const std::function<void(VkCommandBuffer cmdBuffer)> &drawCommands, uint32_t frameIdx);
    void recordCommandBuffer(const std::function<void(VkCommandBuffer cmdBuffer)> &drawCommands, uint32_t frameIdx);

protected:
    std::vector<const char *> getRequiredInstanceExtensions();
    SwapChainSupportDetails   querySwapChainSupport(VkPhysicalDevice device);
    void                      immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);
    void                      loadImageFromFile(vkl::Texture &texture, std::string_view imagePath);
    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);

protected:
    Device *m_device;

protected:
    const std::string m_sessionName;

    GLFWwindow               *m_window = nullptr;
    VkInstance                m_instance;
    std::vector<const char *> m_supportedInstanceExtensions;

    VkDebugUtilsMessengerEXT m_debugMessenger;

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

    vkl::Texture m_depthAttachment;

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

    bool m_framebufferResized = false;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIdx;

    WindowData   m_windowData;
    PerFrameData m_frameData;
    MouseData    m_mouseData;
    Camera       m_camera;

    vkl::PipelineBuilder m_pipelineBuilder;

    vkl::DeletionQueue m_deletionQueue;
};
} // namespace vkl

#endif // VULKANBASE_H_
