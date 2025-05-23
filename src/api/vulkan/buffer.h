#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct BufferCreateInfo
{
    std::size_t size       = { 0 };
    BufferUsageFlags usage = {};
    MemoryDomain domain    = { MemoryDomain::Auto };
};

class Buffer : public ResourceHandle<::vk::Buffer, BufferCreateInfo>
{
    friend class ThreadSafeObjectPool<Buffer>;
    friend class CommandBuffer;

public:
    auto getSize() const -> std::size_t;
    auto getUsage() const -> BufferUsageFlags;

private:
    Buffer(const CreateInfoType& createInfo, HandleType handle);
};
} // namespace aph::vk
