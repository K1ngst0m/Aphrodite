#include "vklSceneManger.h"

namespace vkl {
Scene &Scene::setCamera(vkl::Camera *camera, UniformBufferObject *ubo) {
    _camera = new SceneCameraNode(ubo, camera);
    return *this;
}

Scene& Scene::pushUniform(UniformBufferObject *ubo)
{
    _uniformNodeList.push_back(new SceneUniformNode(ubo));
    return *this;
}

Scene& Scene::pushEntity(Entity *object, ShaderPass *pass, glm::mat4 transform, SCENE_RENDER_TYPE renderType)
{
    _renderNodeList.push_back(new SceneRenderNode(object, pass, transform));
    return *this;
}

uint32_t Scene::getRenderableCount() const {
    return _renderNodeList.size();
}
uint32_t Scene::getUBOCount() const {
    return _uniformNodeList.size() + 1;
}
} // namespace vkl
