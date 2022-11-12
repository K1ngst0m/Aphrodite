#include "uniformObject.h"
#include "scene/uniformObject.h"
#include "device.h"

namespace vkl {

void VulkanUniformData::updateBuffer(void *data) const {
    _buffer->copyTo(data, _buffer->getSize());
}
VulkanUniformData::VulkanUniformData(VulkanDevice * device, std::shared_ptr<UniformObject> ubo)
    : _device(device), _ubo(std::move(ubo)) {
    _ubo->load();
    setupBuffer(_ubo->getDataSize(), _ubo->getData());
}

void VulkanUniformData::setupBuffer(uint32_t bufferSize, void *data) {
    BufferCreateInfo createInfo{};
    createInfo.size = bufferSize;
    createInfo.usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VK_CHECK_RESULT(_device->createBuffer(&createInfo, &_buffer, data));
    _buffer->setupDescriptor();
    _buffer->map();
}

void VulkanUniformData::cleanupResources() const {
    _device->destroyBuffer(_buffer);
}

void *VulkanUniformData::getData() {
    return _ubo->getData();
}
VkDescriptorBufferInfo &VulkanUniformData::getBufferInfo() {
    return _buffer->getBufferInfo();
}
} // namespace vkl
