#ifndef SCENENODE_H_
#define SCENENODE_H_

#include "object.h"

namespace aph
{
template <typename TNode>
struct Node : public Object, std::enable_shared_from_this<TNode>
{
    Node(std::shared_ptr<TNode> parent, IdType id, ObjectType type, glm::mat4 transform = glm::mat4(1.0f)) :
        Object{ id, type },
        parent{ std::move(parent) },
        matrix{ transform }
    {
        if constexpr(std::is_same<TNode, Node>::value)
        {
            if(parent->parent)
            {
                name = parent->name + "-" + std::to_string(id);
            }
            else
            {
                name = std::to_string(id);
            }
        }
    }
    std::shared_ptr<TNode> createChildNode(glm::mat4 transform = glm::mat4(1.0f))
    {
        auto childNode = std::make_shared<TNode>(this->shared_from_this(), transform);
        children.push_back(childNode);
        return childNode;
    }
    void addChild(std::shared_ptr<TNode> childNode) { children.push_back(std::move(childNode)); }
    std::string name{};
    std::vector<std::shared_ptr<TNode>> children{};
    std::shared_ptr<TNode> parent{ nullptr };
    glm::mat4 matrix{ 1.0f };
};

class Camera;
class Light;
class Mesh;

struct SceneNode : Node<SceneNode>
{
    SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix = glm::mat4(1.0f));
    IdType getAttachObjectId() { return m_object->getId(); }

    template <typename TObject>
    void attachObject(const std::shared_ptr<Object> &object)
    {
        if constexpr(isObjectTypeValid<TObject>())
        {
            m_attachType = object->getType();
            m_object = object;
        }
        else
        {
            assert("Invalid type of the object.");
        }
    }

    template <typename TObject>
    std::shared_ptr<TObject> getObject()
    {
        if constexpr(isObjectTypeValid<TObject>())
        {
            return std::static_pointer_cast<TObject>(m_object);
        }
        else
        {
            assert("Invalid type of the object.");
        }
    }

    template <typename TObject>
    constexpr static bool isObjectTypeValid()
    {
        return std::is_same<TObject, Camera>::value || std::is_same<TObject, Light>::value ||
               std::is_same<TObject, Mesh>::value;
    }

    std::shared_ptr<Object> m_object{ nullptr };
    ObjectType m_attachType{ ObjectType::UNATTACHED };
};
}  // namespace aph

#endif  // SCENENODE_H_
