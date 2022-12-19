#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "renderer/renderer.h"
#include "scene/scene.h"

namespace vkl
{
class SceneRenderer
{
public:
    SceneRenderer() = default;
    virtual ~SceneRenderer() = default;

    virtual void loadResources() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void drawScene() = 0;

    virtual void cleanupResources() = 0;

    ShadingModel getShadingModel() const { return m_shadingModel; }
    void setShadingModel(ShadingModel model) { m_shadingModel = model; }
    void setScene(const std::shared_ptr<Scene> &scene) { m_scene = scene; }

protected:
    std::shared_ptr<Scene> m_scene = nullptr;
    ShadingModel m_shadingModel = ShadingModel::UNLIT;
};

}  // namespace vkl

#endif  // VKLSCENERENDERER_H_
