#include "uniformObject.h"

#include "device.h"

namespace vkl {

void VulkanUniformObject::updateBuffer(void *data) const {
    buffer.copyTo(data, buffer.size);
}
VulkanUniformObject::VulkanUniformObject(SceneRenderer *renderer, const std::shared_ptr<VulkanDevice> &device, vkl::UniformObject *ubo)
    : _device(device), _renderer(renderer), _ubo(ubo) {
}

void VulkanUniformObject::setupBuffer(uint32_t bufferSize, void *data) {
    _device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, data);
    buffer.setupDescriptor();
    buffer.map();
}

void VulkanUniformObject::cleanupResources() const {
    buffer.destroy();
}

} // namespace vkl
