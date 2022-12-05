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
void SceneNode::attachObject(const std::shared_ptr<Entity> &object)
{
    attachType = AttachType::ENTITY;
    this->object = object;
}
void SceneNode::attachObject(const std::shared_ptr<Light> &object)
{
    attachType = AttachType::LIGHT;
    this->object = object;
}
void SceneNode::attachObject(const std::shared_ptr<Camera> &object)
{
    attachType = AttachType::CAMERA;
    this->object = object;
}

}  // namespace vkl
