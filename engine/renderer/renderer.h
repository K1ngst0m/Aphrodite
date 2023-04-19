#ifndef RENDERER_H_
#define RENDERER_H_

#include <utility>

#include "api/gpuResource.h"
#include "common/common.h"
#include "common/window.h"

namespace aph
{
struct RenderConfig
{
    bool             enableDebug         = { true };
    bool             enableUI            = { true };
    bool             initDefaultResource = { true };
    uint32_t         maxFrames           = { 2 };
    SampleCountFlags sampleCount         = { SAMPLE_COUNT_1_BIT };
};

class VulkanSceneRenderer;
class VulkanRenderer;
class IRenderer
{
public:
    template <typename TRenderer>
    static std::unique_ptr<TRenderer> Create(const std::shared_ptr<Window>& window, const RenderConfig& config)
    {
        std::unique_ptr<TRenderer> renderer = {};
        if constexpr(std::is_same<TRenderer, VulkanRenderer>::value)
        {
            renderer = std::make_unique<VulkanRenderer>(window, config);
        }
        else if constexpr(std::is_same<TRenderer, VulkanSceneRenderer>::value)
        {
            renderer = std::make_unique<VulkanSceneRenderer>(window, config);
        }
        else
        {
            assert("current type of the renderer is not supported.");
        }
        return renderer;
    }
    IRenderer(std::shared_ptr<Window> window, const RenderConfig& config) :
        m_window(std::move(window)),
        m_config(config)
    {
    }

    std::shared_ptr<Window> getWindow() { return m_window; }
    uint32_t                getWindowWidth() { return m_window->getWidth(); };
    uint32_t                getWindowHeight() { return m_window->getHeight(); };
    uint32_t                getWindowAspectRation() { return m_window->getAspectRatio(); }

    virtual void cleanup()    = 0;
    virtual void idleDevice() = 0;

protected:
    std::shared_ptr<Window> m_window = {};
    RenderConfig            m_config = {};
};
}  // namespace aph

#endif  // RENDERER_H_
