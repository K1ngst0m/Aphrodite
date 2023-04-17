#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "renderer/renderer.h"
#include "scene/scene.h"

namespace aph
{
class VulkanSceneRenderer;
class ISceneRenderer
{
public:
    virtual void loadResources()           = 0;
    virtual void update(float deltaTime)   = 0;
    virtual void recordDrawSceneCommands() = 0;

    virtual void cleanupResources() = 0;

    ShadingModel getShadingModel() const { return m_shadingModel; }
    void         setShadingModel(ShadingModel model) { m_shadingModel = model; }
    void         setScene(const std::shared_ptr<Scene>& scene) { m_scene = scene; }

protected:
    std::shared_ptr<Scene> m_scene        = {};
    ShadingModel           m_shadingModel = { ShadingModel::PBR };
};

}  // namespace aph

#endif  // VKLSCENERENDERER_H_
