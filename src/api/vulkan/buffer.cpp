#include "buffer.h"

namespace aph::vk
{

Buffer::Buffer(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
{
}

auto Buffer::getSize() const -> std::size_t
{
    return m_createInfo.size;
}

auto Buffer::getUsage() const -> BufferUsageFlags
{
    return m_createInfo.usage;
}

} // namespace aph::vk
