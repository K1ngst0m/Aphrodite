#include "entity.h"
#include "entityGLTFLoader.h"

namespace vkl {
void Entity::loadFromFile(const std::string &path) {
    _loader = new EntityGLTFLoader(this, path);
    _loader->load();

}
Entity::~Entity() {
    for (Image *image : _images) {
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
vkl::ShaderPass *Entity::getPass() {
    return _pass;
}
} // namespace vkl
