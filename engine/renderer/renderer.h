#ifndef RENDERER_H_
#define RENDERER_H_

#include <utility>

#include "common/common.h"
#include "common/window.h"

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
    Renderer(std::shared_ptr<WindowData>  windowData, const RenderConfig &config) :
        m_windowData(std::move(windowData)),
        m_config(config)
    {
    }
    void setWindowData(const std::shared_ptr<WindowData> &windowData) { m_windowData = windowData; }
    uint32_t getWindowHeight() { return m_windowData->height; };
    uint32_t getWindowWidth() { return m_windowData->width; };
    uint32_t getWindowAspectRation() { return m_windowData->getAspectRatio(); }

    virtual void cleanup() = 0;
    virtual void idleDevice() = 0;

protected:
    std::shared_ptr<WindowData> m_windowData = nullptr;
    RenderConfig m_config;
};
}  // namespace vkl

#endif  // RENDERER_H_
