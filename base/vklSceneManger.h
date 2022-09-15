#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "vklCamera.h"
#include "vklObject.h"

#include <map>

namespace vkl {

enum class SCENE_UNIFORM_TYPE : uint8_t {
    UNDEFINED,
    CAMERA,
    POINT_LIGHT,
    DIRECTIONAL_LIGHT,
    FLASH_LIGHT,
};

enum class SCENE_RENDER_TYPE : uint8_t {
    OPAQUE,
    TRANSPARENCY,
};

struct SceneNode {
    std::vector<SceneNode *> _children;
};

struct SceneRenderNode : SceneNode {
    vkl::RenderObject *_object;
    vkl::ShaderPass   *_pass;
    glm::mat4 _transform;

    SceneRenderNode(vkl::RenderObject *object, vkl::ShaderPass *pass, glm::mat4 transform)
        : _object(object), _pass(pass), _transform(transform) {
    }
};

struct SceneUniformNode : SceneNode {
    SCENE_UNIFORM_TYPE _type;

    vkl::UniformBufferObject *_object = nullptr;

    SceneUniformNode(vkl::UniformBufferObject *object, SCENE_UNIFORM_TYPE uniformType = SCENE_UNIFORM_TYPE::UNDEFINED)
        : _type(uniformType), _object(object) {
    }
};

struct SceneCameraNode : SceneUniformNode {
    SceneCameraNode(vkl::UniformBufferObject *object, vkl::Camera *camera)
        : SceneUniformNode(object, SCENE_UNIFORM_TYPE::CAMERA), _camera(camera) {
    }

    vkl::Camera *_camera;
};

class Scene {
public:
    Scene &pushUniform(UniformBufferObject *ubo);
    Scene &pushCamera(vkl::Camera *camera, UniformBufferObject *ubo);
    Scene &pushMeshObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f), SCENE_RENDER_TYPE renderType = SCENE_RENDER_TYPE::OPAQUE);

    uint32_t getRenderableCount() const {
        return _renderNodeList.size();
    }
    uint32_t getUBOCount() const {
        return _uniformNodeList.size() + _cameraNodeList.size();
    }

public:
    std::vector<SceneRenderNode *>  _renderNodeList;
    std::vector<SceneUniformNode *> _uniformNodeList;
    std::vector<SceneCameraNode *>  _cameraNodeList;
};
} // namespace vkl

#endif // VKLSCENEMANGER_H_
