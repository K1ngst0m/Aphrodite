#ifndef RENDERER_H_
#define RENDERER_H_

#include "api/gpuResource.h"
#include "common/common.h"
#include "wsi/wsi.h"

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
    RENDER_CFG_WITHOUT_UI  = RENDER_CFG_DEBUG | RENDER_CFG_DEFAULT_RES,
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
    static std::unique_ptr<TRenderer> Create(WSI* window, const RenderConfig& config)
    {
        std::unique_ptr<TRenderer> renderer = {};
        if constexpr(std::is_same_v<TRenderer, vk::Renderer>)
        {
            CM_LOG_INFO("Init Common Renderer.");
            renderer = std::make_unique<vk::Renderer>(window, config);
        }
        else if constexpr(std::is_same_v<TRenderer, vk::SceneRenderer>)
        {
            CM_LOG_INFO("Init Scene Renderer.");
            renderer = std::make_unique<vk::SceneRenderer>(window, config);
        }
        else
        {
            CM_LOG_ERR("Current type of the renderer is not supported.");
            APH_ASSERT(false);
        }
        return renderer;
    }
    IRenderer(WSI* wsi, const RenderConfig& config) : m_wsi(wsi), m_config(config) {}
    virtual ~IRenderer() = default;

public:
    virtual void load()                  = 0;
    virtual void unload()                = 0;
    virtual void update(float deltaTime) = 0;

    WSI*     getWSI() const { return m_wsi; }
    uint32_t getWindowWidth() const { return m_wsi->getWidth(); };
    uint32_t getWindowHeight() const { return m_wsi->getHeight(); };

    const RenderConfig& getConfig() const { return m_config; }

protected:
    WSI*         m_wsi    = {};
    RenderConfig m_config = {};
};
}  // namespace aph

#endif  // RENDERER_H_
