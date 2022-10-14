#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "idObject.h"

namespace vkl {
class SceneNode;
class Object;
class Entity;
class EntityLoader;
class Light;
class Camera;
struct Primitive;
struct Texture;
struct Material;
struct SubEntity;
struct Vertex;


enum class ShadingModel {
    UNLIT,
    DEFAULTLIT,
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

enum PrefabEntity : IdType {
    PREFAB_ENTITY_PLANE  = 0,
    PREFAB_ENTITY_BOX    = 1,
    PREFAB_ENTITY_SPHERE = 2,
};

using CameraMapList = std::unordered_map<IdType, std::shared_ptr<Camera>>;
using EntityMapList = std::unordered_map<IdType, std::shared_ptr<Entity>>;
using LightMapList  = std::unordered_map<IdType, std::shared_ptr<Light>>;

enum class SceneManagerType {
    DEFAULT,
};

class SceneManager {
public:
    static std::unique_ptr<SceneManager> Create(SceneManagerType type);

    SceneManager();
    ~SceneManager();

    void update(float deltaTime);

public:
    std::shared_ptr<Light>      createLight();
    std::shared_ptr<Entity>     createEntity();
    std::shared_ptr<Entity>     createEntity(const std::string &path);
    std::shared_ptr<Camera>     createCamera(float aspectRatio);
    std::unique_ptr<SceneNode> &getRootNode();

    std::shared_ptr<Entity> getEntityWithId(IdType id);
    std::shared_ptr<Camera> getCameraWithId(IdType id);
    std::shared_ptr<Light>  getLightWithId(IdType id);

public:
    void      setMainCamera(const std::shared_ptr<Camera> &camera);
    void      setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    void _createPrefabEntity();

    AABB      aabb;
    glm::vec4 _ambient;

    std::unique_ptr<SceneNode> _rootNode;
    std::shared_ptr<Camera>    _camera = nullptr;

    CameraMapList _cameraMapList;
    EntityMapList _entityMapList;
    LightMapList  _lightMapList;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
