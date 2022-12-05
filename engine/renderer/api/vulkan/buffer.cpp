#include "buffer.h"
#include "device.h"

namespace vkl {
VulkanBuffer *VulkanBuffer::CreateFromHandle(VulkanDevice *pDevice, BufferCreateInfo *pCreateInfo, VkBuffer buffer, VkDeviceMemory memory) {
    VulkanBuffer *instance = new VulkanBuffer;
    memcpy(&instance->getCreateInfo(), pCreateInfo, sizeof(BufferCreateInfo));
    instance->device = pDevice->getHandle();
    instance->_handle = buffer;
    instance->memory = memory;

    return instance;
}

VkResult VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
    return vkMapMemory(device, memory, offset, size, 0, &mapped);
}


void VulkanBuffer::unmap() {
    if (mapped) {
        vkUnmapMemory(device, memory);
        mapped = nullptr;
    }
}

VkResult VulkanBuffer::bind(VkDeviceSize offset) const {
    return vkBindBufferMemory(device, _handle, memory, offset);
}

void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
    descriptorInfo.offset = offset;
    descriptorInfo.buffer = _handle;
    descriptorInfo.range  = size;
}

void VulkanBuffer::copyTo(const void *data, VkDeviceSize size) const {
    assert(mapped);
    memcpy(mapped, data, size);
}

void VulkanBuffer::copyTo(const void *data) const
{
    copyTo(data, getSize());
}

VkResult VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
}

}  // namespace vkl
