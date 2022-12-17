#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "node.h"

namespace vkl
{
enum class ShadingModel
{
    UNLIT,
    DEFAULTLIT,
    PBR,
};

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

enum class SceneManagerType
{
    DEFAULT,
};

class Light;
class Entity;
class Camera;

using CameraMapList = std::unordered_map<IdType, std::shared_ptr<Camera>>;
using EntityMapList = std::unordered_map<IdType, std::shared_ptr<Entity>>;
using LightMapList = std::unordered_map<IdType, std::shared_ptr<Light>>;

class Scene
{
public:
    static std::unique_ptr<Scene> Create(SceneManagerType type);

    std::shared_ptr<Light> createLight();
    std::shared_ptr<Entity> createEntity();
    std::shared_ptr<Entity> createEntityFromGLTF(const std::string &path);
    std::shared_ptr<Camera> createCamera(float aspectRatio);

    void setAmbient(glm::vec3 value) { m_ambient = value; }
    void setMainCamera(const std::shared_ptr<Camera> &camera) { m_camera = camera; }

    std::shared_ptr<SceneNode> getRootNode() { return m_rootNode; }
    std::shared_ptr<Light> getLightWithId(IdType id) { return m_lightMapList[id]; }
    std::shared_ptr<Camera> getCameraWithId(IdType id) { return m_cameraMapList[id]; }
    std::shared_ptr<Entity> getEntityWithId(IdType id) { return m_entityMapList[id]; }
    glm::vec3 getAmbient() { return m_ambient; }

private:
    AABB m_aabb;
    glm::vec3 m_ambient = glm::vec3(0.02f);

    std::shared_ptr<SceneNode> m_rootNode = nullptr;
    std::shared_ptr<Camera> m_camera = nullptr;

    CameraMapList m_cameraMapList;
    EntityMapList m_entityMapList;
    LightMapList m_lightMapList;
};

}  // namespace vkl

#endif  // VKLSCENEMANGER_H_
