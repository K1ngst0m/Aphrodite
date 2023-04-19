#include "buffer.h"
#include "device.h"

namespace aph
{

VulkanBuffer::VulkanBuffer(const BufferCreateInfo& createInfo, VkBuffer buffer, VkDeviceMemory memory) : memory(memory)
{
    getHandle()     = buffer;
    getCreateInfo() = createInfo;
}

void VulkanBuffer::write(const void* data, size_t offset, VkDeviceSize size) const
{
    assert(mapped);
    if(size == VK_WHOLE_SIZE)
    {
        size = getSize();
    }
    uint8_t* pMapped = (uint8_t*)mapped;
    memcpy(pMapped + offset, data, size);
}

}  // namespace aph
