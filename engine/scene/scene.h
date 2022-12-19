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
class Camera;
struct Mesh;
struct ImageInfo;
struct Material;

using CameraMap = std::unordered_map<IdType, std::shared_ptr<Camera>>;
using LightMap = std::unordered_map<IdType, std::shared_ptr<Light>>;
using MeshMap = std::unordered_map<IdType, std::shared_ptr<Mesh>>;
using ImageInfoList = std::vector<std::shared_ptr<ImageInfo>>;
using MaterialList = std::vector<std::shared_ptr<Material>>;

class Scene
{
public:
    static std::unique_ptr<Scene> Create(SceneManagerType type);

    std::shared_ptr<Mesh> createMesh();
    std::shared_ptr<Light> createLight();
    std::shared_ptr<Camera> createCamera(float aspectRatio);
    std::shared_ptr<SceneNode> createMeshesFromFile(const std::string &path, const std::shared_ptr<SceneNode> &parent = nullptr);

    void setAmbient(glm::vec3 value) { m_ambient = value; }
    void setMainCamera(const std::shared_ptr<Camera> &camera) { m_camera = camera; }
    std::shared_ptr<Camera> getMainCamera() { return m_camera; }

    std::shared_ptr<SceneNode> getRootNode() { return m_rootNode; }
    std::shared_ptr<Light> getLightWithId(IdType id) { return m_lights[id]; }
    std::shared_ptr<Camera> getCameraWithId(IdType id) { return m_cameras[id]; }
    std::shared_ptr<Mesh> getMeshWithId(IdType id) { return m_meshes[id]; }

    glm::vec3 getAmbient() { return m_ambient; }

    ImageInfoList &getImages() { return m_images; }
    MaterialList &getMaterials() { return m_materials; }

private:
    AABB m_aabb;
    glm::vec3 m_ambient = glm::vec3(0.02f);

    std::shared_ptr<SceneNode> m_rootNode = nullptr;
    std::shared_ptr<Camera> m_camera = nullptr;

    CameraMap m_cameras;
    LightMap m_lights;
    MeshMap m_meshes;
    ImageInfoList m_images;
    MaterialList m_materials;
};

}  // namespace vkl

#endif  // VKLSCENEMANGER_H_
