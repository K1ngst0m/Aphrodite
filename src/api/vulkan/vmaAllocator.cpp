#include "vmaAllocator.h"

#include "api/vulkan/device.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace aph::vk
{
VMADeviceAllocator::VMADeviceAllocator(Instance* pInstance, Device* pDevice)
{
    auto& table = VULKAN_HPP_DEFAULT_DISPATCHER;

    VmaVulkanFunctions vulkanFunctions = {
        .vkGetInstanceProcAddr                   = table.vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                     = table.vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties           = table.vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = table.vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = table.vkAllocateMemory,
        .vkFreeMemory                            = table.vkFreeMemory,
        .vkMapMemory                             = table.vkMapMemory,
        .vkUnmapMemory                           = table.vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = table.vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = table.vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = table.vkBindBufferMemory,
        .vkBindImageMemory                       = table.vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = table.vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = table.vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = table.vkCreateBuffer,
        .vkDestroyBuffer                         = table.vkDestroyBuffer,
        .vkCreateImage                           = table.vkCreateImage,
        .vkDestroyImage                          = table.vkDestroyImage,
        .vkCmdCopyBuffer                         = table.vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR       = table.vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR        = table.vkGetImageMemoryRequirements2,
        .vkBindBufferMemory2KHR                  = table.vkBindBufferMemory2,
        .vkBindImageMemory2KHR                   = table.vkBindImageMemory2,
        .vkGetPhysicalDeviceMemoryProperties2KHR = table.vkGetPhysicalDeviceMemoryProperties2,
        .vkGetDeviceBufferMemoryRequirements     = table.vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = table.vkGetDeviceImageMemoryRequirements,
    };

    VmaAllocatorCreateInfo allocatorCreateInfo = { .flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                                   .physicalDevice = static_cast<VkPhysicalDevice>(
                                                       pDevice->getPhysicalDevice()->getHandle()),
                                                   .device           = static_cast<VkDevice>(pDevice->getHandle()),
                                                   .pVulkanFunctions = &vulkanFunctions,
                                                   .instance         = static_cast<VkInstance>(pInstance->getHandle()),
                                                   .vulkanApiVersion = VK_API_VERSION_1_3 };

    vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
}

VMADeviceAllocator::~VMADeviceAllocator()
{
    clear();
    vmaDestroyAllocator(m_allocator);
}

auto VMADeviceAllocator::allocate(Buffer* pBuffer) -> DeviceAllocation*
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(!m_bufferMemoryMap.contains(pBuffer));

    VmaAllocationCreateInfo allocCreateInfo = getAllocationCreateInfo(pBuffer);
    VmaAllocationInfo allocInfo;
    VmaAllocation allocation;
    vmaAllocateMemoryForBuffer(m_allocator, pBuffer->getHandle(), &allocCreateInfo, &allocation, &allocInfo);
    vmaBindBufferMemory(m_allocator, allocation, pBuffer->getHandle());
    auto [it, inserted] = m_bufferMemoryMap.insert({ pBuffer, VMADeviceAllocation(allocation, allocInfo) });
    if (const auto& name = pBuffer->getDebugName(); !name.empty())
    {
        vmaSetAllocationName(m_allocator, allocation, name.data());
    }
    return &it->second;
}

auto VMADeviceAllocator::allocate(Image* pImage) -> DeviceAllocation*
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(!m_imageMemoryMap.contains(pImage));

    VmaAllocationCreateInfo allocCreateInfo = getAllocationCreateInfo(pImage);
    VmaAllocationInfo allocInfo;
    VmaAllocation allocation;
    vmaAllocateMemoryForImage(m_allocator, pImage->getHandle(), &allocCreateInfo, &allocation, &allocInfo);
    vmaBindImageMemory(m_allocator, allocation, pImage->getHandle());
    auto [it, inserted] = m_imageMemoryMap.insert({ pImage, VMADeviceAllocation(allocation, allocInfo) });
    if (const auto& name = pImage->getDebugName(); !name.empty())
    {
        vmaSetAllocationName(m_allocator, allocation, name.data());
    }
    return &it->second;
}

auto VMADeviceAllocator::free(Image* pImage) -> void
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    vmaFreeMemory(m_allocator, m_imageMemoryMap.find(pImage)->second.getHandle());
    m_imageMemoryMap.erase(pImage);
}

auto VMADeviceAllocator::free(Buffer* pBuffer) -> void
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    vmaFreeMemory(m_allocator, m_bufferMemoryMap.find(pBuffer)->second.getHandle());
    m_bufferMemoryMap.erase(pBuffer);
}

auto VMADeviceAllocator::map(Buffer* pBuffer, void** ppData) -> Result
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    return utils::getResult(vmaMapMemory(m_allocator, m_bufferMemoryMap.find(pBuffer)->second.getHandle(), ppData));
}

auto VMADeviceAllocator::map(Image* pImage, void** ppData) -> Result
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    return utils::getResult(vmaMapMemory(m_allocator, m_imageMemoryMap.find(pImage)->second.getHandle(), ppData));
}

auto VMADeviceAllocator::unMap(Buffer* pBuffer) -> void
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    vmaUnmapMemory(m_allocator, m_bufferMemoryMap.find(pBuffer)->second.getHandle());
}

auto VMADeviceAllocator::unMap(Image* pImage) -> void
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    vmaUnmapMemory(m_allocator, m_imageMemoryMap.find(pImage)->second.getHandle());
}

auto VMADeviceAllocator::clear() -> void
{
    for (auto& [image, allocation] : m_imageMemoryMap)
    {
        free(image);
    }
    for (auto& [buffer, allocation] : m_bufferMemoryMap)
    {
        free(buffer);
    }
}

auto VMADeviceAllocator::flush(Image* pImage, Range range) -> Result
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(
        vmaFlushAllocation(m_allocator, m_imageMemoryMap.find(pImage)->second.getHandle(), range.offset, range.size));
}

auto VMADeviceAllocator::flush(Buffer* pBuffer, Range range) -> Result
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(
        vmaFlushAllocation(m_allocator, m_bufferMemoryMap.find(pBuffer)->second.getHandle(), range.offset, range.size));
}

auto VMADeviceAllocator::invalidate(Image* pImage, Range range) -> Result
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(vmaInvalidateAllocation(m_allocator, m_imageMemoryMap.find(pImage)->second.getHandle(),
                                                    range.offset, range.size));
}

auto VMADeviceAllocator::invalidate(Buffer* pBuffer, Range range) -> Result
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(vmaInvalidateAllocation(m_allocator, m_bufferMemoryMap.find(pBuffer)->second.getHandle(),
                                                    range.offset, range.size));
}

auto VMADeviceAllocator::getAllocationCreateInfo(MemoryDomain memoryDomain, bool deviceAccess)
    -> VmaAllocationCreateInfo
{
    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; // Default to unknown usage

    switch (memoryDomain)
    {
    // TODO
    case MemoryDomain::Auto:
    case MemoryDomain::Device:
    {
        allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        allocationCreateInfo.flags         = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }
    break;
    case MemoryDomain::Host:
    {
        allocationCreateInfo.requiredFlags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocationCreateInfo.flags          = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                     VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                     VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    break;
    case MemoryDomain::Upload:
    {
        allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocationCreateInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    break;
    case MemoryDomain::Readback:
    {
        allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    break;
    }

    return allocationCreateInfo;
}

auto VMADeviceAllocator::getAllocationCreateInfo(Image* pImage) -> VmaAllocationCreateInfo
{
    APH_ASSERT(pImage);
    const auto& imageCreateInfo = pImage->getCreateInfo();
    bool deviceAccess = static_cast<bool>(imageCreateInfo.usage & ~(ImageUsage::TransferDst | ImageUsage::TransferSrc));
    VmaAllocationCreateInfo allocationCreateInfo = getAllocationCreateInfo(imageCreateInfo.domain, deviceAccess);
    return allocationCreateInfo;
}

auto VMADeviceAllocator::getAllocationCreateInfo(Buffer* pBuffer) -> VmaAllocationCreateInfo
{
    APH_ASSERT(pBuffer);
    const auto& bufferCreateInfo = pBuffer->getCreateInfo();
    bool deviceAccess =
        static_cast<bool>(bufferCreateInfo.usage & ~(BufferUsage::TransferDst | BufferUsage::TransferSrc));
    VmaAllocationCreateInfo allocCreateInfo = getAllocationCreateInfo(bufferCreateInfo.domain, deviceAccess);
    return allocCreateInfo;
}

} // namespace aph::vk
