#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "idObject.h"

namespace vkl
{

class Scene;

enum class ObjectType : uint8_t
{
    UNATTACHED,
    ENTITY,
    LIGHT,
    CAMERA,
    MESH,
};

class Object : public IdObject
{
public:
    // template <typename TObject, typename... Args>
    // static std::shared_ptr<TObject> Create(Args &&...args)
    // {
    //     auto instance = std::make_shared<TObject>(std::forward<Args>(args)...);
    // }
    Object(IdType id, ObjectType type) : IdObject(id), m_type(type) {}
    virtual ~Object() = default;

    ObjectType getType() { return m_type; }

protected:
    ObjectType m_type;
};

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

}  // namespace vkl

#endif  // VKLMODEL_H_
