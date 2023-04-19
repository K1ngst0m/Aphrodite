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
    uint8_t* pData = (uint8_t*)data;
    memcpy(mapped, pData + offset, size);
}

}  // namespace aph
