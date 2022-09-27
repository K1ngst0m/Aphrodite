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
    _textures.clear();
    _subEntityList.clear();
    _materials.clear();
}
void Entity::setShadingModel(ShadingModel type) {
    _shadingModel = type;
}
ShadingModel Entity::getShadingModel() const {
    return _shadingModel;
}
} // namespace vkl
