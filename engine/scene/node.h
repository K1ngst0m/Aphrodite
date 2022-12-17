#ifndef SCENENODE_H_
#define SCENENODE_H_

#include "object.h"

namespace vkl
{
template <typename TNode>
struct Node : std::enable_shared_from_this<TNode>
{
    Node(std::shared_ptr<TNode> parent, glm::mat4 transform = glm::mat4(1.0f)) :
        parent(std::move(parent)),
        matrix(transform)
    {
    }
    std::shared_ptr<TNode> createChildNode(glm::mat4 transform = glm::mat4(1.0f))
    {
        auto childNode = std::make_shared<TNode>(this->shared_from_this(), transform);
        children.push_back(childNode);
        return childNode;
    }
    std::string name;
    std::vector<std::shared_ptr<TNode>> children;
    std::shared_ptr<TNode> parent;
    glm::mat4 matrix = glm::mat4(1.0f);
};

struct SceneNode : Node<SceneNode>
{
    SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix = glm::mat4(1.0f));
    void attachObject(const std::shared_ptr<Object> &object);
    template <typename TObject>
    std::shared_ptr<TObject> getObject()
    {
        return std::static_pointer_cast<TObject>(m_object);
    }
    IdType getAttachObjectId() { return m_object->getId(); }

    std::shared_ptr<Object> m_object = nullptr;
    ObjectType m_attachType = ObjectType::UNATTACHED;
};
}  // namespace vkl

#endif  // SCENENODE_H_
