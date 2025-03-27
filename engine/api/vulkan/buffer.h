#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class Buffer;

struct BufferCreateInfo
{
    std::size_t size = { 0 };
    BufferUsageFlags usage = {};
    MemoryDomain domain = { MemoryDomain::Auto };
};

class Buffer : public ResourceHandle<::vk::Buffer, BufferCreateInfo>
{
    friend class ObjectPool<Buffer>;
    friend class CommandBuffer;

public:
    uint32_t getSize() const
    {
        return m_createInfo.size;
    }
    ResourceState getResourceState() const
    {
        return m_resourceState;
    }

private:
    Buffer(const CreateInfoType& createInfo, HandleType handle);
    ResourceState m_resourceState = ResourceState::Undefined;
};
} // namespace aph::vk
