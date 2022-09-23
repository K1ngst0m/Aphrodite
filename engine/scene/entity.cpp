#include "entity.h"
#include "entityGLTFLoader.h"

namespace vkl {
void Entity::loadFromFile(const std::string &path) {
    _loader = new EntityGLTFLoader(this, path);
    _loader->load();

}
Entity::~Entity() {
    for (auto * subEntity : _subEntityList){
        delete subEntity;
    }
}
Entity::Entity(SceneManager *manager)
    : Object(manager) {
}
EntityLoader::EntityLoader(Entity *entity)
    : _entity(entity) {
}
} // namespace vkl
