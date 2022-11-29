#include "scene.h"

#include <memory>
#include "camera.h"
#include "common/assetManager.h"
#include "entity.h"
#include "light.h"
#include "sceneNode.h"

namespace vkl {
std::unique_ptr<Scene> Scene::Create(SceneManagerType type) {
    switch (type) {
    case SceneManagerType::DEFAULT: {
        return std::make_unique<Scene>();
    }
    default:{
        assert("scene manager type not support.");
        return {};
    }
    }
}

Scene::Scene()
    : _ambient(glm::vec4(0.2f)) {
    _rootNode = std::make_unique<SceneNode>(nullptr);
}

std::shared_ptr<Camera> Scene::createCamera(float aspectRatio) {
    auto camera = Camera::Create();
    camera->setAspectRatio(aspectRatio);
    _cameraMapList[camera->getId()] = camera;
    return camera;
}

std::shared_ptr<Light> Scene::createLight() {
    auto light                    = Light::Create();
    _lightMapList[light->getId()] = light;
    return light;
}

std::shared_ptr<Entity> Scene::createEntity() {
    auto entity                     = Entity::Create();
    _entityMapList[entity->getId()] = entity;
    return entity;
}

void Scene::setAmbient(glm::vec3 value) {
    _ambient = value;
}

glm::vec3 Scene::getAmbient() {
    // TODO IDK why this return _ambient causes segment fault
    return glm::vec3(0.2f);
}

Scene::~Scene() = default;

std::shared_ptr<SceneNode> Scene::getRootNode() {
    return _rootNode;
}
std::shared_ptr<Entity> Scene::createEntityFromGLTF(const std::string &path) {
    auto entity = createEntity();
    entity->loadFromFile(path);
    return entity;
}
void Scene::setMainCamera(const std::shared_ptr<Camera> &camera) {
    _camera = camera;
}
std::shared_ptr<Light> Scene::getLightWithId(IdType id) {
    return _lightMapList[id];
}
std::shared_ptr<Camera> Scene::getCameraWithId(IdType id) {
    return _cameraMapList[id];
}
std::shared_ptr<Entity> Scene::getEntityWithId(IdType id) {
    return _entityMapList[id];
}
} // namespace vkl
