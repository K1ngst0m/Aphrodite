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
struct Mesh;
struct ImageDesc;
struct Material;

using CameraMap = std::unordered_map<IdType, std::shared_ptr<Camera>>;
using EntityMap = std::unordered_map<IdType, std::shared_ptr<Entity>>;
using LightMap = std::unordered_map<IdType, std::shared_ptr<Light>>;
using MeshMap = std::unordered_map<IdType, std::shared_ptr<Mesh>>;

class Scene
{
public:
    static std::unique_ptr<Scene> Create(SceneManagerType type);

    std::shared_ptr<Mesh> createMesh();
    std::shared_ptr<Light> createLight();
    std::shared_ptr<Entity> createEntity();
    std::shared_ptr<Entity> createEntityFromFile(const std::string &path);
    std::shared_ptr<Camera> createCamera(float aspectRatio);
    std::shared_ptr<SceneNode> createFromFile(const std::string &path,
                                              const std::shared_ptr<SceneNode> &parent = nullptr);

    void setAmbient(glm::vec3 value) { m_ambient = value; }
    void setMainCamera(const std::shared_ptr<Camera> &camera) { m_camera = camera; }
    std::shared_ptr<Camera> getMainCamera() { return m_camera; }

    std::shared_ptr<SceneNode> getRootNode() { return m_rootNode; }
    std::shared_ptr<Light> getLightWithId(IdType id) { return m_lights[id]; }
    std::shared_ptr<Camera> getCameraWithId(IdType id) { return m_cameras[id]; }
    std::shared_ptr<Entity> getEntityWithId(IdType id) { return m_entities[id]; }
    std::shared_ptr<Mesh> getMeshWithId(IdType id) { return m_meshes[id]; }
    std::vector<std::shared_ptr<ImageDesc>> &getImages() { return m_images; }
    std::vector<std::shared_ptr<Material>> &getMaterials() { return m_materials; }
    glm::vec3 getAmbient() { return m_ambient; }

private:
    AABB m_aabb;
    glm::vec3 m_ambient = glm::vec3(0.02f);

    std::shared_ptr<SceneNode> m_rootNode = nullptr;
    std::shared_ptr<Camera> m_camera = nullptr;

    CameraMap m_cameras;
    EntityMap m_entities;
    LightMap m_lights;
    MeshMap m_meshes;
    std::vector<std::shared_ptr<ImageDesc>> m_images;
    std::vector<std::shared_ptr<Material>> m_materials;
};

}  // namespace vkl

#endif  // VKLSCENEMANGER_H_
