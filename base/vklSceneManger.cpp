#include "vklSceneManger.h"

namespace vkl {
SceneCamera* SceneManager::createCamera(float aspectRatio) {
    SceneCamera* camera = new SceneCamera(aspectRatio);
    _camera = new SceneCameraNode(camera);
    return camera;
}

Light* SceneManager::createLight()
{
    Light *ubo = new Light();
    _lightNodeList.push_back(new SceneLightNode(ubo));
    return ubo;
}

Entity* SceneManager::createEntity(ShaderPass *pass, glm::mat4 transform, SCENE_RENDER_TYPE renderType)
{
    Entity * entity = new Entity();
    _renderNodeList.push_back(new SceneEntityNode(entity, pass, transform));
    return entity;
}

uint32_t SceneManager::getRenderableCount() const {
    return _renderNodeList.size();
}
uint32_t SceneManager::getUBOCount() const {
    return _lightNodeList.size() + 1;
}
} // namespace vkl
