#include "object.h"

namespace vkl {
UniformBufferObject::UniformBufferObject(SceneManager *manager)
    : Object(manager) {
}
void UniformBufferObject::setupBuffer(vkl::Device *device, uint32_t bufferSize, void *data) {
    device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, data);
    buffer.setupDescriptor();
}
void UniformBufferObject::updateBuffer(void *data) {
    buffer.update(data);
}
void UniformBufferObject::destroy() {
    buffer.destroy();
}
} // namespace vkl
