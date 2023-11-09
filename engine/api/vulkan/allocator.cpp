#include "allocator.h"
#include "image.h"
#include "buffer.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace aph::vk
{
const HashMap<ImageDomain, VmaMemoryUsage> m_imageDomainUsageMap = {
    {ImageDomain::Device, VMA_MEMORY_USAGE_GPU_ONLY},
    {ImageDomain::Transient, VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED},
    {ImageDomain::LinearHost, VMA_MEMORY_USAGE_CPU_TO_GPU},
    {ImageDomain::LinearHostCached, VMA_MEMORY_USAGE_GPU_TO_CPU},
};
const HashMap<BufferDomain, VmaMemoryUsage> m_bufferDomainUsageMap = {
    {BufferDomain::Device, VMA_MEMORY_USAGE_GPU_ONLY},
    {BufferDomain::LinkedDeviceHost, VMA_MEMORY_USAGE_CPU_TO_GPU},
    {BufferDomain::Host, VMA_MEMORY_USAGE_CPU_ONLY},
    {BufferDomain::CachedHost, VMA_MEMORY_USAGE_GPU_TO_CPU},
};

VMADeviceAllocator::VMADeviceAllocator(Instance* pInstance, Device* pDevice)
{
    auto& table = *pDevice->getDeviceTable();

    VmaVulkanFunctions vulkanFunctions = {
        .vkGetInstanceProcAddr                   = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                     = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
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
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
        .vkGetDeviceBufferMemoryRequirements     = table.vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = table.vkGetDeviceImageMemoryRequirements,
    };

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice         = pDevice->getPhysicalDevice()->getHandle();
    allocatorCreateInfo.device                 = pDevice->getHandle();
    allocatorCreateInfo.instance               = pInstance->getHandle();
    allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
}

VMADeviceAllocator::~VMADeviceAllocator()
{
    vmaDestroyAllocator(m_allocator);
}

DeviceAllocation* VMADeviceAllocator::allocate(Buffer* pBuffer)
{
    APH_ASSERT(!m_bufferMemoryMap.contains(pBuffer));

    const auto& bufferCI = pBuffer->getCreateInfo();

    VmaMemoryUsage usage = VMA_MEMORY_USAGE_UNKNOWN;
    if(m_bufferDomainUsageMap.contains(bufferCI.domain))
    {
        usage = m_bufferDomainUsageMap.at(bufferCI.domain);
    }

    VmaAllocationCreateInfo allocCreateInfo = {.usage = usage};
    VmaAllocationInfo       allocInfo;
    VmaAllocation           allocation;
    vmaAllocateMemoryForBuffer(m_allocator, pBuffer->getHandle(), &allocCreateInfo, &allocation, &allocInfo);
    vmaBindBufferMemory(m_allocator, allocation, pBuffer->getHandle());
    DeviceAllocation* pAllocation = new VMADeviceAllocation{allocation, allocInfo};
    m_bufferMemoryMap[pBuffer]    = pAllocation;
    return pAllocation;
}
DeviceAllocation* VMADeviceAllocator::allocate(Image* pImage)
{
    APH_ASSERT(!m_imageMemoryMap.contains(pImage));

    const auto& bufferCI = pImage->getCreateInfo();

    VmaMemoryUsage usage = VMA_MEMORY_USAGE_UNKNOWN;
    if(m_imageDomainUsageMap.contains(bufferCI.domain))
    {
        usage = m_imageDomainUsageMap.at(bufferCI.domain);
    }

    VmaAllocationCreateInfo allocCreateInfo = {.usage = usage};
    VmaAllocationInfo       allocInfo;
    VmaAllocation           allocation;
    vmaAllocateMemoryForImage(m_allocator, pImage->getHandle(), &allocCreateInfo, &allocation, &allocInfo);
    vmaBindImageMemory(m_allocator, allocation, pImage->getHandle());
    DeviceAllocation* pAllocation = new VMADeviceAllocation{allocation, allocInfo};
    m_imageMemoryMap[pImage]      = pAllocation;
    return pAllocation;
}
void VMADeviceAllocator::free(Image* pImage)
{
    APH_ASSERT(m_imageMemoryMap.contains(pImage));
    auto alloc = static_cast<VMADeviceAllocation*>(m_imageMemoryMap[pImage]);
    vmaFreeMemory(m_allocator, alloc->getHandle());
    m_imageMemoryMap.erase(pImage);
}
void VMADeviceAllocator::free(Buffer* pBuffer)
{
    APH_ASSERT(m_bufferMemoryMap.contains(pBuffer));
    auto alloc = static_cast<VMADeviceAllocation*>(m_bufferMemoryMap[pBuffer]);
    vmaFreeMemory(m_allocator, alloc->getHandle());
    m_bufferMemoryMap.erase(pBuffer);
}
Result VMADeviceAllocator::map(Buffer* pBuffer, void** ppData)
{
    auto alloc = static_cast<VMADeviceAllocation*>(m_bufferMemoryMap[pBuffer]);
    return utils::getResult(vmaMapMemory(m_allocator, alloc->getHandle(), ppData));
}
Result VMADeviceAllocator::map(Image* pImage, void** ppData)
{
    auto alloc = static_cast<VMADeviceAllocation*>(m_imageMemoryMap[pImage]);
    return utils::getResult(vmaMapMemory(m_allocator, alloc->getHandle(), ppData));
}
void VMADeviceAllocator::unMap(Buffer* pBuffer)
{
    auto alloc = static_cast<VMADeviceAllocation*>(m_bufferMemoryMap[pBuffer]);
    vmaUnmapMemory(m_allocator, alloc->getHandle());
}
void VMADeviceAllocator::unMap(Image* pImage)
{
    auto alloc = static_cast<VMADeviceAllocation*>(m_imageMemoryMap[pImage]);
    vmaUnmapMemory(m_allocator, alloc->getHandle());
}
void VMADeviceAllocator::clear()
{
    for(auto& [image, _] : m_imageMemoryMap)
    {
        free(image);
    }
    for(auto& [buffer, _] : m_bufferMemoryMap)
    {
        free(buffer);
    }
}
}  // namespace aph::vk
