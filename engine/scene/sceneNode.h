#ifndef SCENENODE_H_
#define SCENENODE_H_

#include "entity.h"
#include "camera.h"
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

    void attachObject(const std::shared_ptr<Entity> &object);
    void attachObject(const std::shared_ptr<Light> &object);
    void attachObject(const std::shared_ptr<Camera> &object);

    template<typename TObject>
    std::shared_ptr<TObject> getObject();

    bool                    isAttached();
    AttachType              getAttachType();
    IdType                  getAttachObjectId();

    std::shared_ptr<SceneNode> createChildNode(glm::mat4 matrix = glm::mat4(1.0f));
    std::vector<std::shared_ptr<SceneNode>>& getChildNode();

    void      setTransform(glm::mat4 matrix);
    glm::mat4 getTransform();

private:
    SceneNode              *_parent;
    std::shared_ptr<Object> _object;

    glm::mat4  _matrix;
    AttachType _attachType = AttachType::UNATTACHED;

    std::vector<std::shared_ptr<SceneNode>> _children;
};

template<typename TObject>
std::shared_ptr<TObject> SceneNode::getObject() {
    return std::static_pointer_cast<TObject>(_object);
}

} // namespace vkl

#endif // SCENENODE_H_
