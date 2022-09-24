#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "sceneNode.h"

namespace vkl {
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    void update();

public:
    std::shared_ptr<Light>       createLight();
    std::shared_ptr<Entity>      createEntity();
    std::shared_ptr<Entity>      createEntity(const std::string &path);
    std::shared_ptr<Camera> createCamera(float aspectRatio);
    SceneNode                   *getRootNode();

public:
    void      setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    AABB                       aabb;
    std::unique_ptr<SceneNode> rootNode;

    std::shared_ptr<Camera> _camera = nullptr;

    glm::vec4 _ambient;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
