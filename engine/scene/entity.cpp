#include "entity.h"
#include "resourceManager.h"

namespace vkl {
Entity::Entity(SceneManager *manager, IdType id)
    : Object(manager, id) {
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
    _subEntityList.clear();
    _materials.clear();
}
} // namespace vkl
