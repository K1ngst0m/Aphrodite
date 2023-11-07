#ifndef APH_VK_MEMORY_H_
#define APH_VK_MEMORY_H_

#include "vkUtils.h"
#include "vk_mem_alloc.h"

namespace aph::vk
{
class Buffer;
class Device;

class DeviceAllocation
{
public:
    virtual ~DeviceAllocation() = 0;

    virtual std::size_t getOffset() = 0;
    virtual std::size_t getSize()   = 0;
};

class DefaultDeviceAllocation final : public DeviceAllocation
{
public:
    ~DefaultDeviceAllocation() override = default;

    std::size_t getOffset() override { return m_offset; }
    std::size_t getSize() override { return m_size; }

private:
    std::size_t    m_offset = 0;
    std::size_t    m_size   = VK_WHOLE_SIZE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};

class VMADeviceAllocation final : public DeviceAllocation
{
public:
    VMADeviceAllocation(VmaAllocation allocation) : m_allocation(allocation) {}
    ~VMADeviceAllocation() override = default;

    std::size_t getOffset() override { return m_offset; }
    std::size_t getSize() override { return m_size; }

private:
    std::size_t   m_offset = 0;
    std::size_t   m_size   = VK_WHOLE_SIZE;
    VmaAllocation m_allocation;
};
}  // namespace aph::vk

#endif  // MEMORY_H_
