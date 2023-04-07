#include "buffer.h"
#include "device.h"

namespace aph {

VulkanBuffer::VulkanBuffer(const BufferCreateInfo &createInfo, VkBuffer buffer, VkDeviceMemory memory)
    :memory(memory)
{
    getHandle() = buffer;
    getCreateInfo() = createInfo;
}

void VulkanBuffer::copyTo(const void *data, VkDeviceSize size) const {
    assert(mapped);
    if (size == VK_WHOLE_SIZE){
        size = getSize();
    }
    memcpy(mapped, data, size);
}

}  // namespace aph
