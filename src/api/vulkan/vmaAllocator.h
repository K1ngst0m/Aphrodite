#pragma once

#include "api/deviceAllocator.h"

#ifndef VMA_ASSERT_LEAK
#define VMA_ASSERT_LEAK(condition)               \
    do                                           \
    {                                            \
        if (!(condition))                        \
        {                                        \
            MM_LOG_ERR("VMA leak detected: "     \
                       "condition (%s) failed.", \
                       #condition);              \
        }                                        \
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
class Instance;
class Device;

class VMADeviceAllocation final : public DeviceAllocation
{
public:
    VMADeviceAllocation(VmaAllocation allocation, const VmaAllocationInfo& allocationInfo)
        : m_allocation(allocation)
        , m_allocationInfo(allocationInfo)
    {
    }

    ~VMADeviceAllocation() override = default;

    auto getOffset() -> std::size_t override
    {
        return m_allocationInfo.offset;
    }

    auto getSize() -> std::size_t override
    {
        return m_allocationInfo.size;
    }

public:
    auto getHandle() const -> VmaAllocation
    {
        return m_allocation;
    }

    auto getInfo() const -> const VmaAllocationInfo&
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
    // Construction/Destruction
    VMADeviceAllocator(Instance* pInstance, Device* pDevice);
    ~VMADeviceAllocator() override;

    // Memory Management
    auto allocate(Buffer* pBuffer) -> DeviceAllocation* override;
    auto allocate(Image* pImage) -> DeviceAllocation* override;
    auto free(Buffer* pBuffer) -> void override;
    auto free(Image* pImage) -> void override;
    auto clear() -> void override;

    // Memory Mapping
    auto map(Buffer* pBuffer, void** ppData) -> Result override;
    auto map(Image* pImage, void** ppData) -> Result override;
    auto unMap(Buffer* pBuffer) -> void override;
    auto unMap(Image* pImage) -> void override;

    // Memory Synchronization
    auto flush(Buffer* pBuffer, Range range = {}) -> Result override;
    auto flush(Image* pImage, Range range = {}) -> Result override;
    auto invalidate(Buffer* pBuffer, Range range = {}) -> Result override;
    auto invalidate(Image* pImage, Range range = {}) -> Result override;

private:
    // Allocation Helpers
    auto getAllocationCreateInfo(Buffer* pBuffer) -> VmaAllocationCreateInfo;
    auto getAllocationCreateInfo(Image* pImage) -> VmaAllocationCreateInfo;
    auto getAllocationCreateInfo(MemoryDomain memoryDomain, bool deviceAccess) -> VmaAllocationCreateInfo;

    // Member Variables
    VmaAllocator m_allocator;
    HashMap<Buffer*, VMADeviceAllocation> m_bufferMemoryMap;
    HashMap<Image*, VMADeviceAllocation> m_imageMemoryMap;
    std::mutex m_allocationLock;
};

} // namespace aph::vk
