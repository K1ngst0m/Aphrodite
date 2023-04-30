#ifndef RENDERER_H_
#define RENDERER_H_

#include <utility>

#include "api/gpuResource.h"
#include "common/common.h"
#include "common/wsi.h"

namespace aph
{

namespace vk
{
class SceneRenderer;
class Renderer;
};  // namespace vk

enum RenderConfigFlagBits
{
    RENDER_CFG_DEBUG       = (1 << 0),
    RENDER_CFG_UI          = (1 << 1),
    RENDER_CFG_DEFAULT_RES = (1 << 2),
    RENDER_CFG_ALL         = RENDER_CFG_DEFAULT_RES | RENDER_CFG_UI
#if defined(APH_DEBUG)
                     | RENDER_CFG_DEBUG
#endif
    ,
};
using RenderConfigFlags = uint32_t;

struct RenderConfig
{
    RenderConfigFlags flags     = RENDER_CFG_ALL;
    uint32_t          maxFrames = {2};
};

class IRenderer
{
public:
    template <typename TRenderer>
    static std::unique_ptr<TRenderer> Create(const std::shared_ptr<WSI>& window, const RenderConfig& config)
    {
        std::unique_ptr<TRenderer> renderer = {};
        if constexpr(std::is_same<TRenderer, vk::Renderer>::value)
        {
            renderer = std::make_unique<vk::Renderer>(window, config);
        }
        else if constexpr(std::is_same<TRenderer, vk::SceneRenderer>::value)
        {
            renderer = std::make_unique<vk::SceneRenderer>(window, config);
        }
        else
        {
            static_assert("current type of the renderer is not supported.");
        }
        return renderer;
    }
    IRenderer(std::shared_ptr<WSI> window, const RenderConfig& config) : m_window(std::move(window)), m_config(config)
    {
    }

    virtual void beginFrame() = 0;
    virtual void endFrame()   = 0;

    std::shared_ptr<WSI> getWindow() { return m_window; }
    uint32_t             getWindowWidth() { return m_window->getWidth(); };
    uint32_t             getWindowHeight() { return m_window->getHeight(); };
    uint32_t             getWindowAspectRation() { return m_window->getAspectRatio(); }

protected:
    std::shared_ptr<WSI> m_window = {};
    RenderConfig         m_config = {};
};
}  // namespace aph

#endif  // RENDERER_H_
