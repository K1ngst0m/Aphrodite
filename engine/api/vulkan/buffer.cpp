#include "buffer.h"
#include "device.h"

namespace aph::vk
{

Buffer::Buffer(const BufferCreateInfo& createInfo, VkBuffer buffer, VkDeviceMemory memory) : m_memory(memory)
{
    getHandle()     = buffer;
    getCreateInfo() = createInfo;
}

}  // namespace aph::vk
