#include "physicalDevice.h"

namespace vkl
{

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle) :
    m_instance(instance)
{
    _handle = handle;

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);
    _queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount,
                                             _queueFamilyProperties.data());
    vkGetPhysicalDeviceProperties(_handle, &_properties);
    vkGetPhysicalDeviceFeatures(_handle, &_features);
    vkGetPhysicalDeviceMemoryProperties(_handle, &_memoryProperties);

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(_handle, nullptr, &extCount, nullptr);
    if(extCount > 0)
    {
        std::vector<VkExtensionProperties> extensions(extCount);
        if(vkEnumerateDeviceExtensionProperties(_handle, nullptr, &extCount, &extensions.front()) ==
           VK_SUCCESS)
        {
            for(auto ext : extensions)
            {
                _supportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }
}

bool VulkanPhysicalDevice::isExtensionSupported(std::string_view extension) const
{
    return (std::find(_supportedExtensions.begin(), _supportedExtensions.end(), extension) !=
            _supportedExtensions.end());
}

uint32_t VulkanPhysicalDevice::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                                              VkBool32 *memTypeFound) const
{
    for(uint32_t i = 0; i < _memoryProperties.memoryTypeCount; i++)
    {
        if((typeBits & 1) == 1)
        {
            if((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
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
        vkGetPhysicalDeviceFormatProperties(_handle, format, &props);
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
}  // namespace vkl
