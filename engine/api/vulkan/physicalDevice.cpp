#include "physicalDevice.h"

namespace aph
{

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice handle)
{
    getHandle() = handle;

    {
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, nullptr);
        assert(queueFamilyCount > 0);
        m_queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, m_queueFamilyProperties.data());
        for(uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
        {
            auto& queueFamily = m_queueFamilyProperties[queueFamilyIndex];
            auto  queueFlags  = queueFamily.queueFlags;
            // universal queue
            if(queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_queueFamilyMap[QueueType::GRAPHICS].push_back(queueFamilyIndex);
            }
            // compute queue
            else if(queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                m_queueFamilyMap[QueueType::COMPUTE].push_back(queueFamilyIndex);
            }
            // transfer queue
            else if(queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                m_queueFamilyMap[QueueType::TRANSFER].push_back(queueFamilyIndex);
            }
        }
    }
    vkGetPhysicalDeviceProperties(getHandle(), &m_properties);
    vkGetPhysicalDeviceMemoryProperties(getHandle(), &m_memoryProperties);

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(getHandle(), nullptr, &extCount, nullptr);
    if(extCount > 0)
    {
        std::vector<VkExtensionProperties> extensions(extCount);
        if(vkEnumerateDeviceExtensionProperties(getHandle(), nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
        {
            for(auto ext : extensions)
            {
                m_supportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }
}

bool VulkanPhysicalDevice::isExtensionSupported(std::string_view extension) const
{
    return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), extension) !=
            m_supportedExtensions.end());
}

uint32_t VulkanPhysicalDevice::findMemoryType(BufferDomain domain, uint32_t mask) const {
	uint32_t prio[3] = {};

	switch (domain)
	{
	case BufferDomain::Device:
		prio[0] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;

	case BufferDomain::LinkedDeviceHost:
		prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[2] = prio[1];
		break;

	case BufferDomain::LinkedDeviceHostPreferDevice:
		prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[1] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		prio[2] = prio[1];
		break;

	case BufferDomain::Host:
		prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		prio[2] = prio[1];
		break;

	case BufferDomain::CachedHost:
		prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		prio[2] = prio[1];
		break;

	case BufferDomain::CachedCoherentHostPreferCached:
		prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		prio[2] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;

	case BufferDomain::CachedCoherentHostPreferCoherent:
		prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		prio[2] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	}

	for (auto &p : prio)
	{
		uint32_t index = findMemoryType(p, mask);
		if (index != UINT32_MAX)
			return index;
	}

	return UINT32_MAX;
}

uint32_t VulkanPhysicalDevice::findMemoryType(VkMemoryPropertyFlags required, uint32_t mask) const
{
    for(uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
    {
        if((1u << i) & mask)
        {
            uint32_t flags = m_memoryProperties.memoryTypes[i].propertyFlags;
            if((flags & required) == required) return i;
        }
    }

    return UINT32_MAX;
}

VkFormat VulkanPhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                   VkFormatFeatureFlags features) const
{
    for(VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(getHandle(), format, &props);
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }

        if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) { return format; }
    }

    assert("failed to find supported format!");
    return {};
}
std::vector<uint32_t> VulkanPhysicalDevice::getQueueFamilyIndexByFlags(QueueType flags)
{
    return m_queueFamilyMap.count(flags) ? m_queueFamilyMap[flags] : std::vector<uint32_t>();
}
size_t VulkanPhysicalDevice::padUniformBufferSize(size_t originalSize) const
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = m_properties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize     = originalSize;
    if(minUboAlignment > 0) { alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1); }
    return alignedSize;
}
}  // namespace aph
