#include "buffer.h"
#include "device.h"

namespace vkl {

VulkanBuffer::VulkanBuffer(VulkanDevice *pDevice, const BufferCreateInfo &createInfo, VkBuffer buffer, VkDeviceMemory memory)
    :pDevice(pDevice), memory(memory)
{
    getHandle() = buffer;
    getCreateInfo() = createInfo;
}

VkResult VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
    return vkMapMemory(pDevice->getHandle(), memory, offset, size, 0, &mapped);
}


void VulkanBuffer::unmap() {
    if (mapped) {
        vkUnmapMemory(pDevice->getHandle(), memory);
        mapped = nullptr;
    }
}

VkResult VulkanBuffer::bind(VkDeviceSize offset) const {
    return vkBindBufferMemory(pDevice->getHandle(), getHandle(), memory, offset);
}

void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
    descriptorInfo.offset = offset;
    descriptorInfo.buffer = getHandle();
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
    return vkFlushMappedMemoryRanges(pDevice->getHandle(), 1, &mappedRange);
}

VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    return vkInvalidateMappedMemoryRanges(pDevice->getHandle(), 1, &mappedRange);
}

}  // namespace vkl
