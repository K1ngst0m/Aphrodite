#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "sceneManager.h"

namespace vkl {
class SceneRenderer;

class RenderObject{
public:
    RenderObject(SceneRenderer * renderer, vkl::Entity* entity)
        : _renderer(renderer), _entity(entity)
    {}
    virtual ~RenderObject() = default;
    virtual void draw() = 0;

    glm::mat4 getTransform() const {return _transform;}
    void setTransform(glm::mat4 transform) {_transform = transform;}

protected:
    glm::mat4 _transform;
    vkl::SceneRenderer * _renderer;
    vkl::Entity * _entity;
};

class SceneRenderer {
public:
    SceneRenderer(SceneManager *sceneManager);

    virtual void loadResources() = 0;
    virtual void update() = 0;
    virtual void drawScene() = 0;

    virtual void cleanupResources() = 0;

    void setScene(SceneManager *scene);

protected:
    SceneManager *_sceneManager;
};

} // namespace vkl

#endif // VKLSCENERENDERER_H_
