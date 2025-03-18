#pragma once

#include "node.h"

namespace aph
{
enum class ShadingModel
{
    PBR,
};

struct AABB
{
    glm::vec3 min = {};
    glm::vec3 max = {};
};

enum class SceneType
{
    DEFAULT,
};

class Scene
{
private:
    Scene() = default;

public:
    static std::unique_ptr<Scene> Create(SceneType type);

    Mesh* createMesh();
    Light* createDirLight(glm::vec3 dir, glm::vec3 color = glm::vec3(1.0f), float intensity = 1.0f);
    Light* createPointLight(glm::vec3 pos, glm::vec3 color = glm::vec3(1.0f), float intensity = 1.0f);
    Light* createLight(LightType type, glm::vec3 color = glm::vec3(1.0f), float intensity = 1.0f);
    Camera* createPerspectiveCamera(float aspectRatio, float fov = 60.0f, float znear = 0.01f, float zfar = 200.0f);
    Camera* createCamera(float aspectRatio, CameraType type);
    SceneNode* createMeshesFromFile(const std::string& path, SceneNode* parent = nullptr);

    SceneNode* getRootNode()
    {
        return m_rootNode.get();
    }
    Camera* getMainCamera()
    {
        return m_camera;
    }
    Light* getLightWithId(IdType id)
    {
        return m_lights[id].get();
    }
    Camera* getCameraWithId(IdType id)
    {
        return m_cameras[id].get();
    }
    Mesh* getMeshWithId(IdType id)
    {
        return m_meshes[id].get();
    }

    std::vector<uint8_t> getIndices() const
    {
        return m_indices;
    }
    std::vector<uint8_t> getVertices() const
    {
        return m_vertices;
    }
    std::vector<Material> getMaterials() const
    {
        return m_materials;
    }
    std::vector<std::shared_ptr<ImageInfo>> getImages() const
    {
        return m_images;
    }
    glm::vec3 getAmbient()
    {
        return m_ambient;
    }

    void setAmbient(glm::vec3 value)
    {
        m_ambient = value;
    }
    void setMainCamera(Camera* camera)
    {
        m_camera = camera;
    }

    void update(float deltaTime);

private:
    AABB m_aabb = {};
    glm::vec3 m_ambient = { 0.02f, 0.02f, 0.02f };

    std::unique_ptr<SceneNode> m_rootNode = {};
    Camera* m_camera = {};

    std::vector<uint8_t> m_indices = {};
    std::vector<uint8_t> m_vertices = {};

    std::unordered_map<IdType, std::unique_ptr<Camera>> m_cameras = {};
    std::unordered_map<IdType, std::unique_ptr<Light>> m_lights = {};
    std::unordered_map<IdType, std::unique_ptr<Mesh>> m_meshes = {};

    std::vector<std::shared_ptr<ImageInfo>> m_images = {};
    std::vector<Material> m_materials = {};
};

} // namespace aph
