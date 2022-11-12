#include "entity.h"
#include "resourceManager.h"

namespace vkl {
Entity::Entity(IdType id)
    : Object(id) {
}
Entity::~Entity() = default;
void Entity::loadFromFile(const std::string &path) {
    if (isLoaded){
        cleanupResources();
    }
    GLTFLoader::load(this, path);
    isLoaded = true;
}
void Entity::cleanupResources() {
    _vertices.clear();
    _indices.clear();
    _images.clear();
    _subNodeList.clear();
    _materials.clear();
}
std::shared_ptr<Entity> Entity::Create() {
    auto instance = std::make_shared<Entity>(Id::generateNewId<Entity>());
    return instance;
}
} // namespace vkl
