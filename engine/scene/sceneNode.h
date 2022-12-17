#ifndef SCENENODE_H_
#define SCENENODE_H_

#include "camera.h"
#include "entity.h"
#include "light.h"

namespace vkl
{
struct SceneNode : Node<SceneNode>
{
    SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix = glm::mat4(1.0f));
    template <typename TObject>
    std::shared_ptr<TObject> getObject() {return std::static_pointer_cast<TObject>(m_object);}
    IdType getAttachObjectId() { return m_object->getId(); }
    void attachObject(const std::shared_ptr<Object> &object);

    std::shared_ptr<Object> m_object = nullptr;
    ObjectType m_attachType = ObjectType::UNATTACHED;
};
}  // namespace vkl

#endif  // SCENENODE_H_
