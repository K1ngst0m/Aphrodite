#include "buffer.h"
#include "device.h"

namespace aph::vk
{

Buffer::Buffer(const CreateInfoType& createInfo, HandleType handle) :
    ResourceHandle(handle, createInfo)
{
}

}  // namespace aph::vk
