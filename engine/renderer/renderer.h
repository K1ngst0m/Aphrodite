#ifndef RENDERER_H_
#define RENDERER_H_

#include "common.h"
#include "device.h"

namespace vkl {
class SceneRenderer;
enum class RenderBackend {
    VULKAN,
    OPENGL,
};

struct WindowData {
    GLFWwindow *window = nullptr;
    uint32_t    width;
    uint32_t    height;
    bool        resized = false;
    WindowData(uint32_t w, uint32_t h)
        : width(w), height(h) {
    }
    float getAspectRatio() const {
        return static_cast<float>(width) * height;
    }
};

class Renderer {
public:
    struct {
        bool           enableDebug = true;
        bool           enableUI    = false;
        const uint32_t maxFrames   = 2;
    } m_settings;

public:
    static std::unique_ptr<Renderer> CreateRenderer(RenderBackend backend);

    virtual void initDevice()    = 0;
    virtual void destroyDevice() = 0;
    virtual void idleDevice()    = 0;
    virtual void prepareFrame()  = 0;
    virtual void submitFrame()   = 0;

    virtual std::shared_ptr<SceneRenderer> getSceneRenderer() = 0;

    void                        setWindowData(const std::shared_ptr<WindowData> &windowData);
    std::shared_ptr<WindowData> getWindowData();

protected:
    std::shared_ptr<GraphicsDevice> _device        = nullptr;
    std::shared_ptr<SceneRenderer>  _sceneRenderer = nullptr;
    std::shared_ptr<WindowData>     _windowData    = nullptr;
};
} // namespace vkl

#endif // RENDERER_H_
