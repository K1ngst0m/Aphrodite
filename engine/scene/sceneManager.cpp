#include "sceneManager.h"
#include "sceneRenderer.h"

namespace vkl {
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
SceneManager::SceneManager()
    : _ambient(glm::vec4(0.2f)) {
}
void SceneManager::destroy() {
    for (auto *node : _renderNodeList) {
        node->_entity->destroy();
        delete node;
    }

    for (auto *node : _lightNodeList) {
        node->_object->destroy();
        delete node;
    }

    _camera->_object->destroy();
    delete _camera;
}
void SceneManager::update() {
    _camera->_object->update();
    renderer->update();
}
} // namespace vkl
