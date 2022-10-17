#ifndef RENDERER_H_
#define RENDERER_H_

#include "common/common.h"
#include "common/window.h"
#include "device.h"

class GLFWwindow;

namespace vkl {
class VulkanSceneRenderer;
class VulkanUIRenderer;

struct RenderConfig {
    bool     enableDebug         = true;
    bool     enableUI            = false;
    bool     initDefaultResource = true;
    uint32_t maxFrames           = 2;
};

class Renderer {
public:
    Renderer(std::shared_ptr<WindowData> windowData, RenderConfig *config);

    virtual void cleanup()        = 0;
    virtual void idleDevice()     = 0;

    void setWindowData(const std::shared_ptr<WindowData> &windowData);

    uint32_t getWindowHeight();
    uint32_t getWindowWidth();
    uint32_t getWindowAspectRation();

protected:
    std::shared_ptr<WindowData>     _windowData    = nullptr;
    RenderConfig                    _config;
};
} // namespace vkl

#endif // RENDERER_H_
