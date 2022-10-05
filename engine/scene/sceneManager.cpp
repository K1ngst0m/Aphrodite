#include "sceneManager.h"
#include "resourceManager.h"

namespace vkl {
SceneManager::SceneManager()
    : _ambient(glm::vec4(0.2f)) {
    _rootNode = std::make_unique<SceneNode>(nullptr);
    _createPrefabEntity();
}

std::shared_ptr<Camera> SceneManager::createCamera(float aspectRatio) {
    auto camera = std::make_shared<Camera>(this, Id::generateNewId<Camera>());
    camera->setAspectRatio(aspectRatio);
    _cameraMapList[camera->getId()] = camera;
    return camera;
}

std::shared_ptr<Light> SceneManager::createLight() {
    auto light                    = std::make_shared<Light>(this, Id::generateNewId<Light>());
    _lightMapList[light->getId()] = light;
    return light;
}

std::shared_ptr<Entity> SceneManager::createEntity() {
    auto entity                     = std::make_shared<Entity>(this, Id::generateNewId<Entity>());
    _entityMapList[entity->getId()] = entity;
    return entity;
}

void SceneManager::setAmbient(glm::vec4 value) {
    _ambient = value;
}

glm::vec4 SceneManager::getAmbient() {
    // TODO IDK why this return _ambient causes segment fault
    return glm::vec4(0.2f);
}

void SceneManager::update() {
    _camera->update();
}

SceneManager::~SceneManager() = default;

std::unique_ptr<SceneNode> &SceneManager::getRootNode() {
    return _rootNode;
}
std::shared_ptr<Entity> SceneManager::createEntity(const std::string &path) {
    auto entity = createEntity();
    entity->loadFromFile(path);
    return entity;
}
void SceneManager::setMainCamera(const std::shared_ptr<Camera> &camera) {
    _camera = camera;
}
std::shared_ptr<Light> SceneManager::getLightWithId(IdType id) {
    return _lightMapList[id];
}
std::shared_ptr<Camera> SceneManager::getCameraWithId(IdType id) {
    return _cameraMapList[id];
}
std::shared_ptr<Entity> SceneManager::getEntityWithId(IdType id) {
    return _entityMapList[id];
}
void SceneManager::_createPrefabEntity() {
    std::filesystem::path modelPath = "assets/models";
    // plane
    auto planeEntity = createEntity(modelPath / "Plane/glTF/Plane.gltf");
    // cube
    auto boxEntity = createEntity(modelPath / "Box/glTF/Box.gltf");
    // sphere
    auto sphereEntity = createEntity(modelPath / "Sphere/glTF/Sphere.gltf");
};
std::unique_ptr<SceneManager> SceneManager::Create(SceneManagerType type) {
    switch (type) {
    case SceneManagerType::DEFAULT:
        return std::make_unique<SceneManager>();
        break;
    }
    assert("scene manager type not support.");
    return {};
}
} // namespace vkl
