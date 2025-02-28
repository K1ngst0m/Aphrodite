#include "deviceAllocator.h"
#include "buffer.h"
#include "image.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace aph::vk
{
const HashMap<ImageDomain, VmaMemoryUsage> m_imageDomainUsageMap = {
    { ImageDomain::Device, VMA_MEMORY_USAGE_GPU_ONLY },
    { ImageDomain::Transient, VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED },
    { ImageDomain::LinearHost, VMA_MEMORY_USAGE_CPU_TO_GPU },
    { ImageDomain::LinearHostCached, VMA_MEMORY_USAGE_GPU_TO_CPU },
};
const HashMap<BufferDomain, VmaMemoryUsage> m_bufferDomainUsageMap = {
    { BufferDomain::Device, VMA_MEMORY_USAGE_GPU_ONLY },
    { BufferDomain::LinkedDeviceHost, VMA_MEMORY_USAGE_CPU_TO_GPU },
    { BufferDomain::Host, VMA_MEMORY_USAGE_CPU_ONLY },
    { BufferDomain::CachedHost, VMA_MEMORY_USAGE_GPU_TO_CPU },
};

VMADeviceAllocator::VMADeviceAllocator(Instance* pInstance, Device* pDevice)
{
    auto& table = VULKAN_HPP_DEFAULT_DISPATCHER;

    VmaVulkanFunctions vulkanFunctions = {
        .vkGetInstanceProcAddr = table.vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = table.vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties = table.vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = table.vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = table.vkAllocateMemory,
        .vkFreeMemory = table.vkFreeMemory,
        .vkMapMemory = table.vkMapMemory,
        .vkUnmapMemory = table.vkUnmapMemory,
        .vkFlushMappedMemoryRanges = table.vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = table.vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = table.vkBindBufferMemory,
        .vkBindImageMemory = table.vkBindImageMemory,
        .vkGetBufferMemoryRequirements = table.vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = table.vkGetImageMemoryRequirements,
        .vkCreateBuffer = table.vkCreateBuffer,
        .vkDestroyBuffer = table.vkDestroyBuffer,
        .vkCreateImage = table.vkCreateImage,
        .vkDestroyImage = table.vkDestroyImage,
        .vkCmdCopyBuffer = table.vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = table.vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR = table.vkGetImageMemoryRequirements2,
        .vkBindBufferMemory2KHR = table.vkBindBufferMemory2,
        .vkBindImageMemory2KHR = table.vkBindImageMemory2,
        .vkGetPhysicalDeviceMemoryProperties2KHR = table.vkGetPhysicalDeviceMemoryProperties2,
        .vkGetDeviceBufferMemoryRequirements = table.vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements = table.vkGetDeviceImageMemoryRequirements,
    };

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = static_cast<VkPhysicalDevice>(pDevice->getPhysicalDevice()->getHandle());
    allocatorCreateInfo.device = static_cast<VkDevice>(pDevice->getHandle());
    allocatorCreateInfo.instance = static_cast<VkInstance>(pInstance->getHandle());
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
}

VMADeviceAllocator::~VMADeviceAllocator()
{
    vmaDestroyAllocator(m_allocator);
    clear();
}

DeviceAllocation* VMADeviceAllocator::allocate(Buffer* pBuffer)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(!m_bufferMemoryMap.contains(pBuffer));

    const auto& bufferCI = pBuffer->getCreateInfo();

    VmaMemoryUsage usage = VMA_MEMORY_USAGE_UNKNOWN;
    if (m_bufferDomainUsageMap.contains(bufferCI.domain))
    {
        usage = m_bufferDomainUsageMap.at(bufferCI.domain);
    }

    VmaAllocationCreateInfo allocCreateInfo = { .usage = usage };
    VmaAllocationInfo allocInfo;
    VmaAllocation allocation;
    vmaAllocateMemoryForBuffer(m_allocator, pBuffer->getHandle(), &allocCreateInfo, &allocation, &allocInfo);
    vmaBindBufferMemory(m_allocator, allocation, pBuffer->getHandle());
    m_bufferMemoryMap[pBuffer] = std::make_unique<VMADeviceAllocation>(allocation, allocInfo);
    if (const auto& name = pBuffer->getDebugName(); !name.empty())
    {
        vmaSetAllocationName(m_allocator, allocation, name.data());
    }
    return m_bufferMemoryMap[pBuffer].get();
}
DeviceAllocation* VMADeviceAllocator::allocate(Image* pImage)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(!m_imageMemoryMap.contains(pImage));

    const auto& bufferCI = pImage->getCreateInfo();

    VmaMemoryUsage usage = VMA_MEMORY_USAGE_UNKNOWN;
    if (m_imageDomainUsageMap.contains(bufferCI.domain))
    {
        usage = m_imageDomainUsageMap.at(bufferCI.domain);
    }

    VmaAllocationCreateInfo allocCreateInfo = { .usage = usage };
    VmaAllocationInfo allocInfo;
    VmaAllocation allocation;
    vmaAllocateMemoryForImage(m_allocator, pImage->getHandle(), &allocCreateInfo, &allocation, &allocInfo);
    vmaBindImageMemory(m_allocator, allocation, pImage->getHandle());
    m_imageMemoryMap[pImage] = std::make_unique<VMADeviceAllocation>(allocation, allocInfo);
    if (const auto& name = pImage->getDebugName(); !name.empty())
    {
        vmaSetAllocationName(m_allocator, allocation, name.data());
    }
    return m_imageMemoryMap[pImage].get();
}
void VMADeviceAllocator::free(Image* pImage)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    vmaFreeMemory(m_allocator, m_imageMemoryMap[pImage]->getHandle());
    m_imageMemoryMap.erase(pImage);
}
void VMADeviceAllocator::free(Buffer* pBuffer)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    vmaFreeMemory(m_allocator, m_bufferMemoryMap[pBuffer]->getHandle());
    m_bufferMemoryMap.erase(pBuffer);
}
Result VMADeviceAllocator::map(Buffer* pBuffer, void** ppData)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    return utils::getResult(vmaMapMemory(m_allocator, m_bufferMemoryMap[pBuffer]->getHandle(), ppData));
}
Result VMADeviceAllocator::map(Image* pImage, void** ppData)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    return utils::getResult(vmaMapMemory(m_allocator, m_imageMemoryMap[pImage]->getHandle(), ppData));
}
void VMADeviceAllocator::unMap(Buffer* pBuffer)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    vmaUnmapMemory(m_allocator, m_bufferMemoryMap[pBuffer]->getHandle());
}
void VMADeviceAllocator::unMap(Image* pImage)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    vmaUnmapMemory(m_allocator, m_imageMemoryMap[pImage]->getHandle());
}
void VMADeviceAllocator::clear()
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
Result VMADeviceAllocator::flush(Image* pImage, MemoryRange range)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(
        vmaFlushAllocation(m_allocator, m_imageMemoryMap[pImage]->getHandle(), range.offset, range.size));
}
Result VMADeviceAllocator::flush(Buffer* pBuffer, MemoryRange range)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(
        vmaFlushAllocation(m_allocator, m_bufferMemoryMap[pBuffer]->getHandle(), range.offset, range.size));
}
Result VMADeviceAllocator::invalidate(Image* pImage, MemoryRange range)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(
        vmaInvalidateAllocation(m_allocator, m_imageMemoryMap[pImage]->getHandle(), range.offset, range.size));
}
Result VMADeviceAllocator::invalidate(Buffer* pBuffer, MemoryRange range)
{
    std::lock_guard<std::mutex> lock{ m_allocationLock };
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    if (range.size == 0)
    {
        range.size = ::vk::WholeSize;
    }
    return utils::getResult(
        vmaInvalidateAllocation(m_allocator, m_bufferMemoryMap[pBuffer]->getHandle(), range.offset, range.size));
}
} // namespace aph::vk
