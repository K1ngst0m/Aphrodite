#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "renderer/renderer.h"
#include "sceneManager.h"

namespace vkl {
class SceneRenderer {
public:
    SceneRenderer(SceneManager *sceneManager);
    virtual ~SceneRenderer() = default;

    virtual void loadResources() = 0;
    virtual void update() = 0;
    virtual void drawScene() = 0;

    virtual void cleanupResources() = 0;

    void setRenderer(Renderer *renderer);
    void setScene(SceneManager *scene);

protected:
    SceneManager *_sceneManager;
    Renderer * _renderer;

    uint32_t frameInFlightCount = 1;
};

} // namespace vkl

#endif // VKLSCENERENDERER_H_
