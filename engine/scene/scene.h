#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

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

    std::shared_ptr<Mesh>              createMesh();
    std::shared_ptr<Light>             createLight();
    std::shared_ptr<PerspectiveCamera> createPerspectiveCamera(float aspectRatio);
    std::shared_ptr<OrthoCamera>       createOrthoCamera(float aspectRatio);
    std::shared_ptr<SceneNode>         createMeshesFromFile(const std::string&                path,
                                                            const std::shared_ptr<SceneNode>& parent = nullptr);

    void setAmbient(glm::vec3 value) { m_ambient = value; }
    void setMainCamera(const std::shared_ptr<Camera>& camera) { m_camera = camera; }

    template <typename CameraType = Camera>
    std::shared_ptr<CameraType> getMainCamera()
    {
        if constexpr(std::is_same<CameraType, PerspectiveCamera>::value ||
                     std::is_same<CameraType, OrthoCamera>::value || std::is_same<CameraType, Camera>::value)
        {
            return std::static_pointer_cast<CameraType>(m_camera);
        }
        else { static_assert("invalid camera type."); }
    }

    std::shared_ptr<SceneNode> getRootNode() { return m_rootNode; }

    std::shared_ptr<Light>  getLightWithId(IdType id) { return m_lights[id]; }
    std::shared_ptr<Camera> getCameraWithId(IdType id) { return m_cameras[id]; }
    std::shared_ptr<Mesh>   getMeshWithId(IdType id) { return m_meshes[id]; }

    std::vector<uint8_t>                    getIndices() const { return m_indices; }
    std::vector<uint8_t>                    getVertices() const { return m_vertices; }
    std::vector<Material>                   getMaterials() const { return m_materials; }
    std::vector<std::shared_ptr<ImageInfo>> getImages() const { return m_images; }

    glm::vec3 getAmbient() { return m_ambient; }

    void update(float deltaTime);

private:
    AABB      m_aabb    = {};
    glm::vec3 m_ambient = {0.02f, 0.02f, 0.02f};

    std::shared_ptr<SceneNode> m_rootNode = {};
    std::shared_ptr<Camera>    m_camera   = {};

    std::vector<uint8_t> m_indices  = {};
    std::vector<uint8_t> m_vertices = {};

    std::unordered_map<IdType, std::shared_ptr<Camera>> m_cameras = {};
    std::unordered_map<IdType, std::shared_ptr<Light>>  m_lights  = {};
    std::unordered_map<IdType, std::shared_ptr<Mesh>>   m_meshes  = {};

    std::vector<std::shared_ptr<ImageInfo>> m_images    = {};
    std::vector<Material>                   m_materials = {};
};

}  // namespace aph

#endif  // VKLSCENEMANGER_H_
