#include "entity.h"
#include "entityGLTFLoader.h"

namespace vkl {
Entity::~Entity() = default;
Entity::Entity(SceneManager *manager)
    : Object(manager) {
}
void Entity::loadFromFile(const std::string &path) {
    GLTFLoader::load(this, path);
}
} // namespace vkl
