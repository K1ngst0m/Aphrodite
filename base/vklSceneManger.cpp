#include "vklSceneManger.h"

namespace vkl {
Camera* SceneManager::createCamera(float aspectRatio, UniformBufferObject *ubo) {
    Camera* camera = new Camera(aspectRatio);
    _camera = new SceneCameraNode(ubo, camera);
    return camera;
}

UniformBufferObject* SceneManager::createUniform()
{
    UniformBufferObject *ubo = new UniformBufferObject();
    _uniformNodeList.push_back(new SceneUniformNode(ubo));
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
    return _uniformNodeList.size() + 1;
}
} // namespace vkl
