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
    glm::mat4  _transform;

    std::vector<SceneNode *> _children;

    SceneNode *createChildNode() {
        SceneNode *childNode = new SceneNode;
        _children.push_back(childNode);
        return childNode;
    }
};

struct SceneEntityNode : SceneNode {
    vkl::Entity     *_entity;
    vkl::ShaderPass *_pass;

    SceneEntityNode(vkl::Entity *entity, vkl::ShaderPass *pass, glm::mat4 transform)
        : _entity(entity), _pass(pass) {
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

class SceneManager {
public:
    // Entity* pushLight(UniformBufferObject *ubo);
    UniformBufferObject* createUniform();
    Entity* createEntity(ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f), SCENE_RENDER_TYPE renderType = SCENE_RENDER_TYPE::OPAQUE);
    Camera* createCamera(float aspectRatio, UniformBufferObject *ubo);

    uint32_t getRenderableCount() const;
    uint32_t getUBOCount() const;

    void destroy(){
        for (auto * node : _renderNodeList){
            node->_entity->destroy();
            delete node;
        }

        for (auto * node: _uniformNodeList){
            node->_object->destroy();
            delete node;
        }
    }

public:
    std::vector<SceneEntityNode *>  _renderNodeList;
    std::vector<SceneUniformNode *> _uniformNodeList;

    SceneCameraNode *_camera = nullptr;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
