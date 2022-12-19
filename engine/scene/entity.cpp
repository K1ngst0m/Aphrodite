#include "entity.h"

namespace vkl
{

Entity::Entity() :
    Object(Id::generateNewId<Entity>(), ObjectType::ENTITY),
    m_rootNode(std::make_shared<SceneNode>(nullptr))
{
}
}  // namespace vkl
