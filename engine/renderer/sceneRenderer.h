#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "renderer/renderer.h"
#include "scene/sceneManager.h"

namespace vkl {
class SceneRenderer {
public:
    SceneRenderer()          = default;
    virtual ~SceneRenderer() = default;

    virtual void loadResources() = 0;
    virtual void update()        = 0;
    virtual void drawScene()     = 0;

    virtual void cleanupResources() = 0;

    void         setScene(const std::shared_ptr<SceneManager> &scene);
    void         setShadingModel(ShadingModel model);
    ShadingModel getShadingModel() const;

protected:
    std::shared_ptr<SceneManager> _sceneManager;
    bool                          isSceneLoaded = false;
    ShadingModel                  _shadingModel = ShadingModel::UNLIT;
};

} // namespace vkl

#endif // VKLSCENERENDERER_H_
