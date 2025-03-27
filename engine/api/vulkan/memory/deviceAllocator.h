#pragma once

#include "api/gpuResource.h"

namespace aph::vk
{
class Buffer;
class Image;

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

    virtual Result map(Buffer* pBuffer, void** ppData) = 0;
    virtual Result map(Image* pImage, void** ppData) = 0;
    virtual void unMap(Buffer* pBuffer) = 0;
    virtual void unMap(Image* pImage) = 0;
    virtual DeviceAllocation* allocate(Buffer* pBuffer) = 0;
    virtual DeviceAllocation* allocate(Image* pImage) = 0;
    virtual void free(Image* pImage) = 0;
    virtual void free(Buffer* pBuffer) = 0;
    virtual Result flush(Image* pImage, Range range) = 0;
    virtual Result flush(Buffer* pBuffer, Range range) = 0;
    virtual Result invalidate(Image* pImage, Range range) = 0;
    virtual Result invalidate(Buffer* pBuffer, Range range) = 0;
    virtual void clear() = 0;
};

} // namespace aph::vk
