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
        vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount,
                                                m_queueFamilyProperties.data());
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
        {
            auto &queueFamily = m_queueFamilyProperties[queueFamilyIndex];
            auto queueFlags = queueFamily.queueFlags;
            // universal queue
            if (queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_queueFamilyMap[QUEUE_GRAPHICS].push_back(queueFamilyIndex);
            }
            // compute queue
            else if(queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                m_queueFamilyMap[QUEUE_COMPUTE].push_back(queueFamilyIndex);
            }
            // transfer queue
            else if(queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                m_queueFamilyMap[QUEUE_TRANSFER].push_back(queueFamilyIndex);
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
        if(vkEnumerateDeviceExtensionProperties(getHandle(), nullptr, &extCount, &extensions.front()) ==
           VK_SUCCESS)
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

uint32_t VulkanPhysicalDevice::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                                              VkBool32 *memTypeFound) const
{
    for(uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
    {
        if((typeBits & 1) == 1)
        {
            if((m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if(memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if(memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }

    throw std::runtime_error("Could not find a matching memory type");
}
VkFormat VulkanPhysicalDevice::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                                   VkImageTiling tiling,
                                                   VkFormatFeatureFlags features) const
{
    for(VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(getHandle(), format, &props);
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }

        if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    assert("failed to find supported format!");
    return {};
}
std::vector<uint32_t> VulkanPhysicalDevice::getQueueFamilyIndexByFlags(QueueTypeFlags flags)
{
    return m_queueFamilyMap.count(flags) ? m_queueFamilyMap[flags] : std::vector<uint32_t>();
}
}  // namespace aph
