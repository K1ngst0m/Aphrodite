#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "sceneNode.h"
#include "api/vulkan/pipeline.h"

namespace vkl {


class SceneRenderer;
class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    void setRenderer(SceneRenderer *renderer);
    void update();

public:
    Light*  createLight();
    Entity* createEntity(ShaderPass *pass = nullptr);
    SceneCamera* createCamera(float aspectRatio);
    SceneNode   *getRootNode();

public:
    void setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    SceneNode * rootNode;

    SceneCamera *_camera = nullptr;
    SceneRenderer * _renderer;

    glm::vec4 _ambient;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
