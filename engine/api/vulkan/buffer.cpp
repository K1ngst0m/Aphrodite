#include "buffer.h"
#include "device.h"

namespace aph {

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

void VulkanBuffer::copyTo(const void *data, VkDeviceSize size) const {
    assert(mapped);
    if (size == VK_WHOLE_SIZE){
        size = getSize();
    }
    memcpy(mapped, data, size);
}

VkResult VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {
        .sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory              = memory,
        .offset              = offset,
        .size                = size,
    };
    return vkFlushMappedMemoryRanges(pDevice->getHandle(), 1, &mappedRange);
}

VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {
        .sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory              = memory,
        .offset              = offset,
        .size                = size,
    };
    return vkInvalidateMappedMemoryRanges(pDevice->getHandle(), 1, &mappedRange);
}

}  // namespace aph
