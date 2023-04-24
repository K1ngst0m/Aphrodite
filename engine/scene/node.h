#ifndef SCENENODE_H_
#define SCENENODE_H_

#include <utility>

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"

namespace aph
{
template <typename TNode>
class Node : public Object
{
public:
    Node(TNode* parent, IdType id, ObjectType type, glm::mat4 transform = glm::mat4(1.0f), std::string name = "") :
        Object{id, type},
        name{std::move(name)},
        parent{parent},
        matrix{transform}
    {
        if constexpr(std::is_same<TNode, Node>::value)
        {
            if(parent->parent) { name = parent->name + "-" + std::to_string(id); }
            else { name = std::to_string(id); }
        }
    }

    TNode* createChildNode(glm::mat4 transform = glm::mat4(1.0f), std::string name = "")
    {
        auto childNode = std::unique_ptr<TNode>(new TNode(static_cast<TNode*>(this), transform, std::move(name)));
        children.push_back(std::move(childNode));
        return childNode.get();
    }

    glm::mat4 getTransform()
    {
        glm::mat4 res         = matrix;
        auto      currentNode = parent;
        while(currentNode)
        {
            res         = currentNode->matrix * res;
            currentNode = currentNode->parent;
        }
        return res;
    }

    void addChild(std::unique_ptr<TNode>&& childNode) { children.push_back(std::move(childNode)); }

    std::vector<TNode*> getChildren() const
    {
        // TODO
        std::vector<TNode*> result;
        for(auto& n : children)
        {
            result.push_back(n.get());
        }
        return result;
    }
    std::string_view getName() const { return name; }

    void rotate(float angle, glm::vec3 axis) { matrix = glm::rotate(matrix, angle, axis); }
    void translate(glm::vec3 value) { matrix = glm::translate(matrix, value); }
    void scale(glm::vec3 value) { matrix = glm::scale(matrix, value); }

protected:
    std::string                         name     = {};
    std::vector<std::unique_ptr<TNode>> children = {};
    TNode*                              parent   = {};
    glm::mat4                           matrix   = {glm::mat4(1.0f)};
};

class SceneNode : public Node<SceneNode>
{
public:
    SceneNode(SceneNode* parent, glm::mat4 matrix = glm::mat4(1.0f), std::string name = "");
    ObjectType getAttachType() const { return m_object ? m_object->getType() : ObjectType::UNATTACHED; };
    IdType     getAttachObjectId() { return m_object->getId(); }

    template <typename TObject>
    void attachObject(Object* object)
    {
        if constexpr(isObjectTypeValid<TObject>()) { m_object = object; }
        else { static_assert("Invalid type of the object."); }
    }

    template <typename TObject>
    TObject* getObject()
    {
        if constexpr(isObjectTypeValid<TObject>()) { return static_cast<TObject*>(m_object); }
        else { static_assert("Invalid type of the object."); }
    }

    template <typename TObject>
    constexpr static bool isObjectTypeValid()
    {
        return std::is_same<TObject, Camera>::value || std::is_same<TObject, Light>::value ||
               std::is_same<TObject, Mesh>::value;
    }

private:
    Object* m_object{};
};
}  // namespace aph

#endif  // SCENENODE_H_
