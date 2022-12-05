#ifndef SCENENODE_H_
#define SCENENODE_H_

#include "camera.h"
#include "entity.h"
#include "light.h"

namespace vkl
{

enum class AttachType : uint8_t
{
    UNATTACHED,
    ENTITY,
    LIGHT,
    CAMERA,
};

struct SceneNode : Node<SceneNode>
{
    SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix = glm::mat4(1.0f));

    void attachObject(const std::shared_ptr<Entity> &object);
    void attachObject(const std::shared_ptr<Light> &object);
    void attachObject(const std::shared_ptr<Camera> &object);
    template <typename TObject>
    std::shared_ptr<TObject> getObject() {return std::static_pointer_cast<TObject>(object);}
    IdType getAttachObjectId() { return object->getId(); }

    std::shared_ptr<Object> object = nullptr;
    AttachType attachType = AttachType::UNATTACHED;
};

}  // namespace vkl

#endif  // SCENENODE_H_
