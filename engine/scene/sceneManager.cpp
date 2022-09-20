#include "sceneManager.h"
#include "sceneRenderer.h"

namespace vkl {
SceneManager::SceneManager()
    : _ambient(glm::vec4(0.2f)) {
}

SceneCamera* SceneManager::createCamera(float aspectRatio) {
    SceneCamera* camera = new SceneCamera(aspectRatio, this);
    _camera = new SceneCameraNode(camera);
    return camera;
}

Light* SceneManager::createLight()
{
    Light *ubo = new Light(this);
    _lightNodeList.push_back(new SceneLightNode(ubo));
    return ubo;
}

Entity* SceneManager::createEntity(ShaderPass *pass, glm::mat4 transform, SCENE_RENDER_TYPE renderType)
{
    Entity * entity = new Entity(this);
    _renderNodeList.push_back(new SceneEntityNode(entity, pass, transform));
    return entity;
}

void SceneManager::setAmbient(glm::vec4 value) {
    _ambient = value;
}

glm::vec4 SceneManager::getAmbient() {
    // TODO IDK why this return _ambient causes segment fault
    return glm::vec4(0.2f);
}

void SceneManager::destroy() {
    for (auto *node : _renderNodeList) {
        delete node;
    }

    for (auto *node : _lightNodeList) {
        delete node;
    }

    delete _camera;
}

void SceneManager::update() {
    _camera->_object->update();
}

void SceneManager::setRenderer(SceneRenderer *renderer) {
    _renderer = renderer;
}
Camera *SceneManager::getSceneCamera() {
    return _camera->_object;
}
SceneLightNode *SceneManager::getLightNode(uint32_t idx) {
    return _lightNodeList[idx];
}
SceneEntityNode *SceneManager::getRenderNode(uint32_t idx) {
    return _renderNodeList[idx];
}
uint32_t SceneManager::getLightNodeCount() {
    return _lightNodeList.size();
}
uint32_t SceneManager::getRenderNodeCount() {
    return _renderNodeList.size();
}
} // namespace vkl
