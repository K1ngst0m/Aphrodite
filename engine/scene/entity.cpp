#include "entity.h"
#include "entityGLTFLoader.h"

namespace vkl {
void Entity::loadFromFile(const std::string &path) {
    _loader = new EntityGLTFLoader(this);
    _loader->loadFromFile(path);

}
Entity::~Entity() {
    for (auto *image : _images) {
        delete image->data;
        delete image;
    }
}
Entity::Entity(SceneManager *manager)
    : Object(manager) {
}
void Entity::setShaderPass(vkl::ShaderPass *pass) {
    _pass = pass;
}
EntityLoader::EntityLoader(Entity *entity)
    : _entity(entity) {
}
} // namespace vkl
