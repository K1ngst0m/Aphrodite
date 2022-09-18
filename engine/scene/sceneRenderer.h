#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "api/device.h"
#include "sceneManager.h"

namespace vkl {
class SceneRenderer;

struct Renderable{
    Renderable(SceneRenderer * renderer, vkl::Entity* entity)
        : _renderer(renderer), entity(entity)
    {}

    virtual void draw() = 0;

    glm::mat4 transform;

    vkl::SceneRenderer * _renderer;
    vkl::Entity * entity;
};

class SceneRenderer {
public:
    SceneRenderer(SceneManager *sceneManager);

    virtual void prepareResource() = 0;
    virtual void drawScene() = 0;

    virtual void destroy() = 0;

    void setScene(SceneManager *scene);

protected:
    SceneManager *_sceneManager;
};

} // namespace vkl

#endif // VKLSCENERENDERER_H_
