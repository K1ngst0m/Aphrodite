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
    std::shared_ptr<Light>      createLight();
    std::shared_ptr<Entity>     createEntity();
    std::shared_ptr<Entity>     createEntity(const std::string &path);
    std::shared_ptr<Camera>     createCamera(float aspectRatio);
    std::unique_ptr<SceneNode> &getRootNode();

public:
    void      setMainCamera(const std::shared_ptr<Camera> &camera);
    void      setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    AABB      aabb;
    glm::vec4 _ambient;

    std::unique_ptr<SceneNode> _rootNode;
    std::shared_ptr<Camera>    _camera = nullptr;

    std::vector<std::shared_ptr<Camera>> _cameraMapList;
    std::vector<std::shared_ptr<Entity>> _entityMapList;
    std::vector<std::shared_ptr<Light>>  _lightMapList;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
