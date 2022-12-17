#include "sceneNode.h"

#include <utility>
#include "camera.h"
#include "entity.h"
#include "light.h"
#include "object.h"

namespace vkl
{

SceneNode::SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix) :
    Node<SceneNode>(std::move(parent), matrix)
{
}
void SceneNode::attachObject(const std::shared_ptr<Object> &object)
{
    assert(object->getType() != ObjectType::UNATTACHED);
    m_attachType = object->getType();
    m_object = object;
}
}  // namespace vkl
