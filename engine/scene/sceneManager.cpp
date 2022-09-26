#include "sceneManager.h"
#include "entityGLTFLoader.h"

namespace vkl {
SceneManager::SceneManager()
    : _ambient(glm::vec4(0.2f)) {
    _rootNode = std::make_unique<SceneNode>(nullptr);
}

std::shared_ptr<Camera> SceneManager::createCamera(float aspectRatio) {
    auto camera = std::make_shared<Camera>(this, Id::generateNewId<Camera>());
    camera->setAspectRatio(aspectRatio);
    _cameraMapList.push_back(camera);
    return camera;
}

std::shared_ptr<Light> SceneManager::createLight() {
    auto light = std::make_shared<Light>(this, Id::generateNewId<Light>());
    _lightMapList.push_back(light);
    return light;
}

std::shared_ptr<Entity> SceneManager::createEntity() {
    auto entity = std::make_shared<Entity>(this, Id::generateNewId<Entity>());
    _entityMapList.push_back(entity);
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

std::unique_ptr<SceneNode>& SceneManager::getRootNode() {
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
} // namespace vkl
