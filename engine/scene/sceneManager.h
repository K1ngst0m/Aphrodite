#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "api/vulkan/pipeline.h"
#include "sceneNode.h"

namespace vkl {
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    void update();

public:
    Light       *createLight();
    Entity      *createEntity();
    Entity      *createEntity(const std::string &path);
    SceneCamera *createCamera(float aspectRatio);
    SceneNode   *getRootNode();

public:
    void      setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    AABB aabb;
    SceneNode *rootNode;

    SceneCamera *_camera = nullptr;

    glm::vec4 _ambient;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
