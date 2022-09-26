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
    WindowData(uint32_t w, uint32_t h)
        : width(w), height(h) {
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
    virtual void initDevice()    = 0;
    virtual void destroyDevice() = 0;
    virtual void idleDevice()    = 0;
    virtual void prepareFrame()  = 0;
    virtual void submitFrame()   = 0;

    virtual std::shared_ptr<SceneRenderer> createSceneRenderer() = 0;

    void        setWindowData(WindowData *windowData);
    WindowData *getWindowData();

    static std::unique_ptr<Renderer> CreateRenderer(RenderBackend backend);

protected:
    GraphicsDevice                *_device;
    std::shared_ptr<SceneRenderer> _sceneRenderer;
    WindowData                    *m_windowData = nullptr;
};
} // namespace vkl

#endif // RENDERER_H_
