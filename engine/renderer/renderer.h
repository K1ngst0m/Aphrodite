#ifndef RENDERER_H_
#define RENDERER_H_

#include "common/common.h"
#include "common/window.h"
#include "device.h"
#include <cstring>
#include <utility>

class GLFWwindow;

namespace vkl {
class SceneRenderer;
class UIRenderer;

enum class RenderBackend {
    VULKAN,
    OPENGL,
};

struct RenderConfig {
    bool     enableDebug         = true;
    bool     enableUI            = false;
    bool     initDefaultResource = true;
    uint32_t maxFrames           = 2;
};

class Renderer {
public:
    static std::unique_ptr<Renderer> Create(RenderBackend backend, RenderConfig *config, std::shared_ptr<WindowData> windowData);

    Renderer(std::shared_ptr<WindowData> windowData, RenderConfig *config);

    virtual void destroy()        = 0;
    virtual void idleDevice()     = 0;
    virtual void drawDemo()       = 0;
    virtual void renderOneFrame() = 0;

    virtual std::shared_ptr<SceneRenderer> getSceneRenderer() = 0;
    virtual std::shared_ptr<UIRenderer>    getUIRenderer()    = 0;

    void setWindowData(const std::shared_ptr<WindowData> &windowData);

    uint32_t getWindowHeight();
    uint32_t getWindowWidth();
    uint32_t getWindowAspectRation();

protected:
    std::shared_ptr<GraphicsDevice> _device        = nullptr;
    std::shared_ptr<SceneRenderer>  _sceneRenderer = nullptr;
    std::shared_ptr<UIRenderer>     _uiRenderer    = nullptr;
    std::shared_ptr<WindowData>     _windowData    = nullptr;
    RenderConfig                    _config;
};
} // namespace vkl

#endif // RENDERER_H_
