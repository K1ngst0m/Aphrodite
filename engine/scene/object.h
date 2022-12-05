#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "idObject.h"

namespace vkl
{

class Scene;

class Object : public IdObject
{
public:
    Object(IdType id) : IdObject(id) {}
    virtual ~Object() = default;
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
