#include "uniformObject.h"

#include "device.h"

namespace vkl {

void VulkanUniformObject::updateBuffer(void *data) const {
    buffer->copyTo(data, buffer->getSize());
}
VulkanUniformObject::VulkanUniformObject(SceneRenderer *renderer, VulkanDevice * device, vkl::UniformObject *ubo)
    : _device(device), _renderer(renderer), _ubo(ubo) {
}

void VulkanUniformObject::setupBuffer(uint32_t bufferSize, void *data) {
    BufferCreateInfo createInfo{};
    createInfo.size = bufferSize;
    createInfo.usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VK_CHECK_RESULT(_device->createBuffer(&createInfo, &buffer, data));
    buffer->setupDescriptor();
    buffer->map();
}

void VulkanUniformObject::cleanupResources() const {
    _device->destroyBuffer(buffer);
}

} // namespace vkl
