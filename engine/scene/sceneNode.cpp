#include "sceneNode.h"
#include "object.h"
#include "light.h"
#include "camera.h"
#include "entity.h"

namespace vkl {

SceneNode::SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix)
    : _matrix(matrix), _parent(std::move(parent)) {
}
void SceneNode::attachObject(const std::shared_ptr<Entity>& object) {
    _attachType = AttachType::ENTITY;
    _object     = object;
}
void SceneNode::attachObject(const std::shared_ptr<Light>& object) {
    _attachType = AttachType::LIGHT;
    _object     = object;
}
void SceneNode::attachObject(const std::shared_ptr<Camera>& object) {
    _attachType = AttachType::CAMERA;
    _object     = object;
}

std::shared_ptr<SceneNode> SceneNode::createChildNode(glm::mat4 matrix) {
    auto childNode = std::make_shared<SceneNode>(shared_from_this(), matrix);
    _children.push_back(childNode);
    return _children.back();
}

void SceneNode::setTransform(glm::mat4 matrix) {
    _matrix = matrix;
}

std::vector<std::shared_ptr<SceneNode>>& SceneNode::getChildNode(){
    return _children;
}

AttachType SceneNode::getAttachType() {
    return _attachType;
}

glm::mat4 SceneNode::getTransform() {
    return _matrix;
}
IdType SceneNode::getAttachObjectId() {
    return _object->getId();
}
bool SceneNode::isAttached() {
    return _attachType != AttachType::UNATTACHED;
}
} // namespace vkl
