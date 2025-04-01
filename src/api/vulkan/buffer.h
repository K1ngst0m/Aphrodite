#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
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
private:
    Buffer(const CreateInfoType& createInfo, HandleType handle);
};
} // namespace aph::vk
