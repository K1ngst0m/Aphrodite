#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "renderer/renderer.h"
#include "scene/scene.h"

namespace aph
{
class ISceneRenderer
{
public:
    template <typename TSceneRenderer, typename... Args>
    static std::unique_ptr<TSceneRenderer> Create(Args &&...args)
    {
        auto instance = std::make_unique<TSceneRenderer>(std::forward<Args>(args)...);
        return instance;
    }
    ISceneRenderer() = default;
    virtual ~ISceneRenderer() = default;

    virtual void loadResources() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void recordDrawSceneCommands() = 0;

    virtual void cleanupResources() = 0;

    ShadingModel getShadingModel() const { return m_shadingModel; }
    void setShadingModel(ShadingModel model) { m_shadingModel = model; }
    void setScene(const std::shared_ptr<Scene> &scene) { m_scene = scene; }

protected:
    std::shared_ptr<Scene> m_scene;
    ShadingModel m_shadingModel{ ShadingModel::PBR };
};

}  // namespace aph

#endif  // VKLSCENERENDERER_H_
