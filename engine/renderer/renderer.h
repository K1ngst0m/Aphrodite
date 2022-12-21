#ifndef RENDERER_H_
#define RENDERER_H_

#include "common/common.h"
#include "common/window.h"
#include "device.h"

class GLFWwindow;

namespace vkl
{
struct RenderConfig
{
    bool enableDebug = true;
    bool enableUI = false;
    bool initDefaultResource = true;
    uint32_t maxFrames = 2;
};

class Renderer
{
public:
    template <typename TRenderer>
    static std::shared_ptr<TRenderer> Create(const std::shared_ptr<WindowData> &windowData,
                                             const RenderConfig &config)
    {
        auto instance = std::make_shared<TRenderer>(windowData, config);
        return instance;
    }
    Renderer(std::shared_ptr<WindowData> windowData, const RenderConfig &config) :
        _windowData(std::move(windowData)),
        _config(config)
    {
    }
    void setWindowData(const std::shared_ptr<WindowData> &windowData) { _windowData = windowData; }
    uint32_t getWindowHeight() { return _windowData->height; };
    uint32_t getWindowWidth() { return _windowData->width; };
    uint32_t getWindowAspectRation() { return _windowData->getAspectRatio(); }

    virtual void cleanup() = 0;
    virtual void idleDevice() = 0;

protected:
    std::shared_ptr<WindowData> _windowData = nullptr;
    RenderConfig _config;
};
}  // namespace vkl

#endif  // RENDERER_H_
