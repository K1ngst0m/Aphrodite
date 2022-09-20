#include "sceneNode.h"

namespace vkl {

SceneNode::SceneNode(SceneNode *parent, glm::mat4 matrix)
    : _parent(parent), _matrix(matrix) {
}
void SceneNode::attachObject(vkl::Entity *object) {
    _attachType = AttachType::ENTITY;
    _object     = object;
}
void SceneNode::attachObject(vkl::Light *object) {
    _attachType = AttachType::LIGHT;
    _object     = object;
}
void SceneNode::attachObject(vkl::SceneCamera *object) {
    _attachType = AttachType::CAMERA;
    _object     = object;
}
SceneNode *SceneNode::createChildNode(glm::mat4 matrix) {
    SceneNode *childNode = new SceneNode(this, matrix);
    _children.push_back(childNode);
    return childNode;
}
void SceneNode::setTransform(glm::mat4 matrix) {
    _matrix = matrix;
}
uint32_t SceneNode::getChildNodeCount() {
    return _children.size();
}
SceneNode *SceneNode::getChildNode(uint32_t idx) {
    return _children[idx];
}
AttachType SceneNode::getAttachType() {
    return _attachType;
}
vkl::Object *SceneNode::getObject() {
    return _object;
}
glm::mat4 SceneNode::getTransform() {
    return _matrix;
}
} // namespace vkl
