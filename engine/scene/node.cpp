#include "node.h"

#include "camera.h"
#include "light.h"
#include "mesh.h"
#include "object.h"

namespace aph
{
void SceneNode::attachObject(const std::shared_ptr<Object> &object)
{
    assert(object->getType() != ObjectType::UNATTACHED);
    m_attachType = object->getType();
    m_object = object;
}
SceneNode::SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix) :
    Node<SceneNode>{ std::move(parent), Id::generateNewId<SceneNode>(), ObjectType::SCENENODE, matrix }
{
}
}  // namespace aph
