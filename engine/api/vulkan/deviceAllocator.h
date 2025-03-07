#pragma once

#include "device.h"

#ifndef VMA_ASSERT_LEAK
#define VMA_ASSERT_LEAK(condition)                        \
     do                                                    \
     {                                                     \
         if (!(condition))                                 \
         {                                                 \
             MM_LOG_ERR("VMA leak detected: "              \
                        "condition (%s) failed.", \
                        #condition);   \
         }                                                 \
     } while (0)
#endif

// Custom leak log macro to print detailed info about leaks.
#ifndef VMA_LEAK_LOG_FORMAT
#define VMA_LEAK_LOG_FORMAT(fmt, ...) MM_LOG_ERR("VMA leak detected: " fmt, __VA_ARGS__)
#endif

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#include "vk_mem_alloc.h"

namespace aph::vk
{
class Buffer;
class Image;
class Device;

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

class VMADeviceAllocation final : public DeviceAllocation
{
public:
    VMADeviceAllocation(VmaAllocation allocation, const VmaAllocationInfo& allocationInfo)
        : m_allocation(allocation)
        , m_allocationInfo(allocationInfo)
    {
    }
    ~VMADeviceAllocation() override = default;

    std::size_t getOffset() override
    {
        return m_allocationInfo.offset;
    }
    std::size_t getSize() override
    {
        return m_allocationInfo.size;
    }

public:
    VmaAllocation getHandle() const
    {
        return m_allocation;
    }
    const VmaAllocationInfo& getInfo() const
    {
        return m_allocationInfo;
    }

private:
    VmaAllocation m_allocation;
    VmaAllocationInfo m_allocationInfo;
};

class VMADeviceAllocator final : public DeviceAllocator
{
public:
    VMADeviceAllocator(const VMADeviceAllocator&) = delete;
    VMADeviceAllocator(VMADeviceAllocator&&) = delete;
    VMADeviceAllocator& operator=(const VMADeviceAllocator&) = delete;
    VMADeviceAllocator& operator=(VMADeviceAllocator&&) = delete;

    VMADeviceAllocator(Instance* pInstance, Device* pDevice);
    ~VMADeviceAllocator() override;

    DeviceAllocation* allocate(Buffer* pBuffer) override;
    DeviceAllocation* allocate(Image* pImage) override;

    void free(Image* pImage) override;
    void free(Buffer* pBuffer) override;

    Result map(Buffer* pBuffer, void** ppData) override;
    Result map(Image* pImage, void** ppData) override;

    void unMap(Buffer* pBuffer) override;
    void unMap(Image* pImage) override;

    Result flush(Image* pImage, Range range = {}) override;
    Result flush(Buffer* pBuffer, Range range = {}) override;
    Result invalidate(Image* pImage, Range range = {}) override;
    Result invalidate(Buffer* pBuffer, Range range = {}) override;

    void clear() override;

private:
    VmaAllocator m_allocator;

    HashMap<Buffer*, std::unique_ptr<VMADeviceAllocation>> m_bufferMemoryMap;
    HashMap<Image*, std::unique_ptr<VMADeviceAllocation>> m_imageMemoryMap;

    std::mutex m_allocationLock;
};

} // namespace aph::vk
