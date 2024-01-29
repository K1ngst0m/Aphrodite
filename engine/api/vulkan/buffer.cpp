#include "buffer.h"

namespace aph::vk
{

Buffer::Buffer(const CreateInfoType& createInfo, HandleType handle, uint64_t deviceAddress) :
    ResourceHandle(handle, createInfo),
    m_deviceAddress(deviceAddress)
{
}

}  // namespace aph::vk
