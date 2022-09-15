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
    if (renderType == SCENE_RENDER_TYPE::TRANSPARENCY){
        float distance = glm::length(_cameraNodeList[0]->_camera->m_position - glm::vec3(transform * glm::vec4({0.0f, 0.0f, 0.0f, 1.0f})));
        _transparentRenderNodeList[distance] = (new SceneRenderNode(object, pass, transform));
    }
    else if(renderType == SCENE_RENDER_TYPE::OPAQUE){
        _opaqueRenderNodeList.push_back(new SceneRenderNode(object, pass, transform));
    }
    return *this;
}

} // namespace vkl
