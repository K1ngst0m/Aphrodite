#ifndef RENDERER_H_
#define RENDERER_H_

#include "common/common.h"
#include "common/window.h"
#include "device.h"

class GLFWwindow;

namespace vkl {
class SceneRenderer;

enum class RenderBackend {
    VULKAN,
    OPENGL,
};

struct RenderConfig {
    bool     enableDebug = true;
    bool     enableUI    = false;
    uint32_t maxFrames   = 2;
};

class Renderer {
public:
    static std::unique_ptr<Renderer> Create(RenderBackend backend, RenderConfig *config, const std::shared_ptr<WindowData> &windowData);

    virtual void init()       = 0;
    virtual void destroy()    = 0;
    virtual void idleDevice() = 0;

    virtual std::shared_ptr<SceneRenderer> getSceneRenderer() = 0;

    void setWindowData(const std::shared_ptr<WindowData> &windowData);

    uint32_t getWindowHeight();
    uint32_t getWindowWidth();
    uint32_t getWindowAspectRation();

protected:
    std::shared_ptr<GraphicsDevice> _device        = nullptr;
    std::shared_ptr<SceneRenderer>  _sceneRenderer = nullptr;
    std::shared_ptr<WindowData>     _windowData    = nullptr;
    RenderConfig                    _config;
};
} // namespace vkl

#endif // RENDERER_H_
