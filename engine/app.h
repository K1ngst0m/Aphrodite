#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include "vkl.hpp"

namespace vkl {

struct MouseData {
    float lastX;
    float lastY;
    bool  firstMouse      = true;
    bool  isCursorDisable = false;
    MouseData(float lastXin, float lastYin)
        : lastX(lastXin), lastY(lastYin) {
    }
};

struct FrameData {
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
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
    // void prepareUI();

protected:
    void initWindow();
    void initVulkan();
    // void initImGui();
    void cleanup();

protected:
    virtual void initDerive() {
    }
    virtual void cleanupDerive() {
    }
    virtual void keyboardHandleDerive();
    virtual void mouseHandleDerive(int xposIn, int yposIn);

    virtual void drawFrame() {
    }

protected:
    const std::string m_sessionName;

    GLFWwindow *m_window = nullptr;

    WindowData   m_windowData;
    FrameData m_frameData;
    MouseData    m_mouseData;

    std::shared_ptr<Camera> m_camera = nullptr;

    vkl::DeletionQueue m_deletionQueue;

    std::unique_ptr<VulkanRenderer> renderer;

    bool m_framebufferResized = false;
};
} // namespace vkl

#endif // VULKANBASE_H_
