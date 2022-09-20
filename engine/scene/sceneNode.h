#ifndef SCENENODE_H_
#define SCENENODE_H_

#include "common.h"
#include "camera.h"
#include "entity.h"
#include "light.h"

namespace vkl {

enum class AttachType : uint8_t {
    UNATTACHED,
    ENTITY,
    LIGHT,
    CAMERA,
};

class SceneNode {
public:
    SceneNode(SceneNode *parent, glm::mat4 matrix = glm::mat4(1.0f));

    void attachObject(vkl::Entity *object);
    void attachObject(vkl::Light *object);
    void attachObject(vkl::SceneCamera *object);

    vkl::Object *getObject();
    AttachType getAttachType();

    SceneNode *createChildNode(glm::mat4 matrix = glm::mat4(1.0f));
    SceneNode *getChildNode(uint32_t idx);
    uint32_t getChildNodeCount();

    void setTransform(glm::mat4 matrix);
    glm::mat4 getTransform();

private:
    SceneNode *_parent;
    vkl::Object * _object;

    glm::mat4  _matrix;
    AttachType _attachType = AttachType::UNATTACHED;

    std::vector<SceneNode *> _children;
};
}

#endif // SCENENODE_H_
