#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "camera.h"
#include "entity.h"
#include "light.h"
#include "api/vulkan/pipeline.h"

namespace vkl {

enum class AttachType : uint8_t {
    UNATTACHED,
    ENTITY,
    LIGHT,
    CAMERA,
};

class SceneNode {
public:
    SceneNode(SceneNode * parent, glm::mat4 matrix = glm::mat4(1.0f))
        : _parent(parent), _matrix(matrix)
    {}

    void attachObject(vkl::Entity * object){
        _attachType = AttachType::ENTITY;
        _object = object;
    }

    void attachObject(vkl::Light * object){
        _attachType = AttachType::LIGHT;
        _object = object;
    }

    void attachObject(vkl::SceneCamera * object){
        _attachType = AttachType::CAMERA;
        _object = object;
    }

    SceneNode *createChildNode(glm::mat4 matrix = glm::mat4(1.0f)) {
        SceneNode *childNode = new SceneNode(this, matrix);
        _children.push_back(childNode);
        return childNode;
    }

    void setTransform(glm::mat4 matrix){
        _matrix = matrix;
    }

    uint32_t getChildNodeCount(){
        return _children.size();
    }

    SceneNode* getChildNode(uint32_t idx){
        return _children[idx];
    }

    AttachType getAttachType(){
        return _attachType;
    }

    vkl::Object * getObject(){
        return _object;
    }

    glm::mat4 getTransform(){
        return _matrix;
    }

private:
    SceneNode *_parent;
    vkl::Object * _object;

    glm::mat4  _matrix;
    AttachType _attachType = AttachType::UNATTACHED;

    std::vector<SceneNode *> _children;
};

class SceneRenderer;
class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    void setRenderer(SceneRenderer *renderer);
    void update();

public:
    Light*  createLight();
    Entity* createEntity(ShaderPass *pass = nullptr);
    SceneCamera* createCamera(float aspectRatio);
    SceneNode   *getRootNode();

public:
    void setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    SceneNode * rootNode;

    SceneCamera *_camera = nullptr;
    SceneRenderer * _renderer;

    glm::vec4 _ambient;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
