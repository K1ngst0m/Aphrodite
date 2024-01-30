#ifndef VULKAN_BUFFER_H_
#define VULKAN_BUFFER_H_

#include <volk.h>
#include "api/gpuResource.h"
#include "allocator/objectPool.h"

namespace aph::vk
{
class Device;
class Buffer;

struct BufferCreateInfo
{
    std::size_t        size      = {0};
    std::size_t        alignment = {0};
    VkBufferUsageFlags usage     = {0};
    BufferDomain       domain    = {BufferDomain::Device};
};

class Buffer : public ResourceHandle<VkBuffer, BufferCreateInfo>
{
    friend class ObjectPool<Buffer>;
    friend class CommandBuffer;

public:
    uint32_t      getSize() const { return m_createInfo.size; }
    uint32_t      getOffset() const { return m_createInfo.alignment; }
    ResourceState getResourceState() const { return m_resourceState; }

private:
    Buffer(const CreateInfoType& createInfo, HandleType handle);
    ResourceState m_resourceState = ResourceState::Undefined;
};
}  // namespace aph::vk

#endif  // VKLBUFFER_H_
