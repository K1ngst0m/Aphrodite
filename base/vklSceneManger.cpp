#include "vklSceneManger.h"

namespace vkl {
Scene &Scene::pushCamera(vkl::Camera *camera, UniformBufferObject *ubo) {
    _cameraNodeList.push_back(new SceneCameraNode(ubo, camera));
    return *this;
}

Scene& Scene::pushUniform(UniformBufferObject *ubo)
{
    _uniformNodeList.push_back(new SceneUniformNode(ubo));
    return *this;
}

Scene& Scene::pushMeshObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform, SCENE_RENDER_TYPE renderType)
{
    _renderNodeList.push_back(new SceneRenderNode(object, pass, transform));
    return *this;
}

} // namespace vkl
