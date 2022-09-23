#include "sceneManager.h"
#include "entityGLTFLoader.h"
#include "sceneRenderer.h"

namespace vkl {
SceneManager::SceneManager()
    : _ambient(glm::vec4(0.2f)) {
    rootNode = std::make_unique<SceneNode>(nullptr);
}

std::shared_ptr<Camera> SceneManager::createCamera(float aspectRatio) {
    _camera = std::make_shared<Camera>(this);
    _camera->setAspectRatio(aspectRatio);
    return _camera;
}

std::shared_ptr<Light> SceneManager::createLight() {
    auto light = std::make_shared<Light>(this);
    return light;
}

std::shared_ptr<Entity> SceneManager::createEntity() {
    auto entity = std::make_shared<Entity>(this);
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

SceneNode *SceneManager::getRootNode() {
    return rootNode.get();
}
std::shared_ptr<Entity> SceneManager::createEntity(const std::string &path) {
    auto entity = std::make_shared<Entity>(this);
    entity->loadFromFile(path);
    return entity;
}
} // namespace vkl
