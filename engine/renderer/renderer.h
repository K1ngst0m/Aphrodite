#ifndef RENDERER_H_
#define RENDERER_H_

#include <utility>

#include "common/common.h"
#include "common/window.h"

namespace aph
{
struct RenderConfig
{
    bool     enableDebug         = { true };
    bool     enableUI            = { false };
    bool     initDefaultResource = { true };
    uint32_t maxFrames           = { 2 };
};

class VulkanRenderer;
class IRenderer
{
public:
    template <typename TRenderer>
    static std::unique_ptr<TRenderer> Create(const std::shared_ptr<WindowData>& windowData, const RenderConfig& config)
    {
        std::unique_ptr<TRenderer> renderer = {};
        if constexpr(std::is_same<TRenderer, VulkanRenderer>::value)
        {
            renderer = std::make_unique<VulkanRenderer>(windowData, config);
        }
        else
        {
            assert("current type of the renderer is not supported.");
        }
        return renderer;
    }
    IRenderer(std::shared_ptr<WindowData> windowData, const RenderConfig& config) :
        m_windowData(std::move(windowData)),
        m_config(config)
    {
    }
    void     setWindowData(const std::shared_ptr<WindowData>& windowData) { m_windowData = windowData; }
    uint32_t getWindowHeight() { return m_windowData->height; };
    uint32_t getWindowWidth() { return m_windowData->width; };
    uint32_t getWindowAspectRation() { return m_windowData->getAspectRatio(); }

    virtual void cleanup()    = 0;
    virtual void idleDevice() = 0;

protected:
    std::shared_ptr<WindowData> m_windowData = {};
    RenderConfig                m_config     = {};
};
}  // namespace aph

#endif  // RENDERER_H_
