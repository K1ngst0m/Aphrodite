#ifndef RENDERER_H_
#define RENDERER_H_

#include <utility>

#include "api/gpuResource.h"
#include "common/common.h"
#include "common/window.h"

namespace aph
{

enum RenderConfigFlagBits
{
    RENDER_CFG_DEBUG       = (1 << 0),
    RENDER_CFG_UI          = (1 << 1),
    RENDER_CFG_DEFAULT_RES = (1 << 2),
    RENDER_CFG_ALL         = RENDER_CFG_DEBUG | RENDER_CFG_UI | RENDER_CFG_DEFAULT_RES,
};
using RenderConfigFlags = uint32_t;

struct RenderConfig
{
    RenderConfigFlags flags     = RENDER_CFG_ALL;
    uint32_t          maxFrames = {2};
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
        else { assert("current type of the renderer is not supported."); }
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

protected:
    std::shared_ptr<Window> m_window = {};
    RenderConfig            m_config = {};
};
}  // namespace aph

#endif  // RENDERER_H_
