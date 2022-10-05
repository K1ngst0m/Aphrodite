#include "buffer.h"
#include "device.h"

namespace vkl {
VulkanBuffer *VulkanBuffer::CreateFromHandle(VulkanDevice *pDevice, BufferCreateInfo *pCreateInfo, VkBuffer buffer, VkDeviceMemory memory) {
    VulkanBuffer *instance = new VulkanBuffer;
    memcpy(&instance->getCreateInfo(), pCreateInfo, sizeof(BufferCreateInfo));
    instance->device = pDevice->getLogicalDevice();
    instance->_handle = buffer;
    instance->memory = memory;

    return instance;
}
/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
VkResult VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
    return vkMapMemory(device, memory, offset, size, 0, &mapped);
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void VulkanBuffer::unmap() {
    if (mapped) {
        vkUnmapMemory(device, memory);
        mapped = nullptr;
    }
}

/**
 * Attach the allocated memory block to the buffer
 *
 * @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
 *
 * @return VkResult of the bindBufferMemory call
 */
VkResult VulkanBuffer::bind(VkDeviceSize offset) const {
    return vkBindBufferMemory(device, _handle, memory, offset);
}

/**
 * Setup the default descriptor for this buffer
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 */
void VulkanBuffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
    descriptorInfo.offset = offset;
    descriptorInfo.buffer = _handle;
    descriptorInfo.range  = size;
}

/**
 * Copies the specified data to the mapped buffer
 *
 * @param data Pointer to the data to copy
 * @param size Size of the data to copy in machine units
 *
 */
void VulkanBuffer::copyTo(const void *data, VkDeviceSize size) const {
    assert(mapped);
    memcpy(mapped, data, size);
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult VulkanBuffer::flush(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult VulkanBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
}

VkDescriptorBufferInfo &VulkanBuffer::getBufferInfo() {
    return descriptorInfo;
}
VkDeviceMemory VulkanBuffer::getMemory() {
    return memory;
}
} // namespace vkl
