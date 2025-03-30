#pragma once

#include "gpuResource.h"

namespace aph::vk
{
    class Buffer;
    class Image;
}

namespace aph
{
class DeviceAllocation
{
public:
    virtual ~DeviceAllocation() = default;

    virtual std::size_t getOffset() = 0;
    virtual std::size_t getSize() = 0;
};

class DeviceAllocator
{
public:
    DeviceAllocator() = default;
    DeviceAllocator(const DeviceAllocator&) = delete;
    DeviceAllocator(DeviceAllocator&&) = delete;
    DeviceAllocator& operator=(const DeviceAllocator&) = delete;
    DeviceAllocator& operator=(DeviceAllocator&&) = delete;

    virtual ~DeviceAllocator() = default;

    virtual Result map(vk::Buffer* pBuffer, void** ppData) = 0;
    virtual Result map(vk::Image* pImage, void** ppData) = 0;
    virtual void unMap(vk::Buffer* pBuffer) = 0;
    virtual void unMap(vk::Image* pImage) = 0;
    virtual DeviceAllocation* allocate(vk::Buffer* pBuffer) = 0;
    virtual DeviceAllocation* allocate(vk::Image* pImage) = 0;
    virtual void free(vk::Image* pImage) = 0;
    virtual void free(vk::Buffer* pBuffer) = 0;
    virtual Result flush(vk::Image* pImage, Range range) = 0;
    virtual Result flush(vk::Buffer* pBuffer, Range range) = 0;
    virtual Result invalidate(vk::Image* pImage, Range range) = 0;
    virtual Result invalidate(vk::Buffer* pBuffer, Range range) = 0;
    virtual void clear() = 0;
};

} // namespace aph::vk
