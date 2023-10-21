#include "buffer.h"
#include "device.h"

namespace aph::vk
{

Buffer::Buffer(const CreateInfoType& createInfo, HandleType handle, VkDeviceMemory memory) :
    ResourceHandle(handle, createInfo),
    m_memory(memory)
{
}

}  // namespace aph::vk
