#pragma once

#include "gpuResource.h"

namespace aph::vk
{
class Buffer;
class Image;
} // namespace aph::vk

namespace aph
{
class DeviceAllocation
{
public:
    virtual ~DeviceAllocation() = default;

    virtual auto getOffset() -> std::size_t = 0;
    virtual auto getSize() -> std::size_t   = 0;
};

class DeviceAllocator
{
public:
    DeviceAllocator()                                  = default;
    DeviceAllocator(const DeviceAllocator&)            = delete;
    DeviceAllocator(DeviceAllocator&&)                 = delete;
    DeviceAllocator& operator=(const DeviceAllocator&) = delete;
    DeviceAllocator& operator=(DeviceAllocator&&)      = delete;

    virtual ~DeviceAllocator() = default;

    virtual auto map(vk::Buffer* pBuffer, void** ppData) -> Result      = 0;
    virtual auto map(vk::Image* pImage, void** ppData) -> Result        = 0;
    virtual auto unMap(vk::Buffer* pBuffer) -> void                     = 0;
    virtual auto unMap(vk::Image* pImage) -> void                       = 0;
    virtual auto allocate(vk::Buffer* pBuffer) -> DeviceAllocation*     = 0;
    virtual auto allocate(vk::Image* pImage) -> DeviceAllocation*       = 0;
    virtual auto free(vk::Image* pImage) -> void                        = 0;
    virtual auto free(vk::Buffer* pBuffer) -> void                      = 0;
    virtual auto flush(vk::Image* pImage, Range range) -> Result        = 0;
    virtual auto flush(vk::Buffer* pBuffer, Range range) -> Result      = 0;
    virtual auto invalidate(vk::Image* pImage, Range range) -> Result   = 0;
    virtual auto invalidate(vk::Buffer* pBuffer, Range range) -> Result = 0;
    virtual auto clear() -> void                                        = 0;
};

} // namespace aph
