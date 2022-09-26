#include "sceneNode.h"

namespace vkl {

SceneNode::SceneNode(SceneNode *parent, glm::mat4 matrix)
    : _parent(parent), _matrix(matrix) {
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

std::unique_ptr<SceneNode>& SceneNode::createChildNode(glm::mat4 matrix) {
    auto childNode = std::make_unique<SceneNode>(this, matrix);
    _children.push_back(std::move(childNode));
    return _children.back();
}

void SceneNode::setTransform(glm::mat4 matrix) {
    _matrix = matrix;
}

uint32_t SceneNode::getChildNodeCount() {
    return _children.size();
}

std::unique_ptr<SceneNode>& SceneNode::getChildNode(uint32_t idx) {
    return _children[idx];
}

AttachType SceneNode::getAttachType() {
    return _attachType;
}

std::shared_ptr<Object> SceneNode::getObject() {
    return _object;
}

glm::mat4 SceneNode::getTransform() {
    return _matrix;
}
} // namespace vkl
