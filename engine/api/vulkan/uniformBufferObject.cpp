#include "uniformBufferObject.h"

namespace vkl {

void VulkanUniformBufferObject::updateBuffer(void *data) {
    buffer.map();
    buffer.copyTo(data, buffer.size);
    buffer.unmap();
}
VulkanUniformBufferObject::VulkanUniformBufferObject(vkl::SceneRenderer *renderer, vkl::VulkanDevice *device, vkl::UniformBufferObject *ubo)
    : _device(device), _renderer(renderer), _ubo(ubo) {
}

void VulkanUniformBufferObject::setupBuffer(uint32_t bufferSize, void *data) {
    _device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, data);
    buffer.setupDescriptor();
}

void VulkanUniformBufferObject::cleanupResources() {
    buffer.destroy();
}

VulkanUniformBufferObject::~VulkanUniformBufferObject() {
    cleanupResources();
}

} // namespace vkl
