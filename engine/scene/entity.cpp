#include "entity.h"
#include "entityGLTFLoader.h"

namespace vkl {
Entity::Entity(SceneManager *manager, IdType id)
    : Object(manager, id) {
}
Entity::~Entity() = default;
void Entity::loadFromFile(const std::string &path) {
    GLTFLoader::load(this, path);
}
} // namespace vkl
