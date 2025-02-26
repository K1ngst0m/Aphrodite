#include "buffer.h"

namespace aph::vk
{

Buffer::Buffer(const CreateInfoType& createInfo, HandleType handle) : ResourceHandle(handle, createInfo)
{
}

}  // namespace aph::vk
