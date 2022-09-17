#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "vklCamera.h"
#include "vklObject.h"

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
    SceneNode *_parent;
    Object    *_object;
    glm::mat4  _transform;

    std::vector<SceneNode *> _children;

    SceneNode *createChildNode() {
        SceneNode *childNode = new SceneNode;
        _children.push_back(childNode);
        return childNode;
    }

    void attachObject(Object *object) {
        _object = object;
    }
};

struct SceneRenderNode : SceneNode {
    vkl::Entity     *_object;
    vkl::ShaderPass *_pass;

    SceneRenderNode(vkl::Entity *object, vkl::ShaderPass *pass, glm::mat4 transform)
        : _object(object), _pass(pass) {
        _transform = transform;
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
    Scene &pushEntity(Entity *object, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f), SCENE_RENDER_TYPE renderType = SCENE_RENDER_TYPE::OPAQUE);

    Scene &setCamera(vkl::Camera *camera, UniformBufferObject *ubo);

    uint32_t getRenderableCount() const;
    uint32_t getUBOCount() const;

public:
    std::vector<SceneRenderNode *>  _renderNodeList;
    std::vector<SceneUniformNode *> _uniformNodeList;

    SceneCameraNode *_camera = nullptr;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
