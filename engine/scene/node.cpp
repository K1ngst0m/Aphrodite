#include "node.h"

#include "camera.h"
#include "light.h"
#include "mesh.h"
#include "object.h"

namespace aph
{
SceneNode::SceneNode(std::shared_ptr<SceneNode> parent, glm::mat4 matrix, std::string name) :
    Node<SceneNode>{ std::move(parent), Id::generateNewId<SceneNode>(), ObjectType::SCENENODE, matrix , std::move(name)}
{
}
}  // namespace aph
