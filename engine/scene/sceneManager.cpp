#include "sceneManager.h"
#include "sceneRenderer.h"

namespace vkl {
SceneManager::SceneManager()
    : _ambient(glm::vec4(0.2f)) {
    rootNode = new SceneNode(nullptr);
}

SceneCamera* SceneManager::createCamera(float aspectRatio) {
    _camera = new SceneCamera(aspectRatio, this);
    return _camera;
}

Light* SceneManager::createLight()
{
    Light *ubo = new Light(this);
    return ubo;
}

Entity* SceneManager::createEntity(ShaderPass *pass)
{
    Entity * entity = new Entity(this);
    if (pass){
        entity->setShaderPass(pass);
    }
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

SceneManager::~SceneManager() {
    delete rootNode;
}

SceneNode *SceneManager::getRootNode() {
    return rootNode;
}
} // namespace vkl
