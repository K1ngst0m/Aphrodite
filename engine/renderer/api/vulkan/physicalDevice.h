#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "instance.h"

namespace vkl
{
class VulkanPhysicalDevice : public ResourceHandle<VkPhysicalDevice>
{
public:
    VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle);

    const VulkanInstance *getInstance() const { return m_instance; }
    const VkPhysicalDeviceProperties &getDeviceProperties() { return _properties; }
    const VkPhysicalDeviceFeatures &getDeviceFeatures() { return _features; }
    const VkPhysicalDeviceMemoryProperties &getMemoryProperties() { return _memoryProperties; }
    const std::vector<std::string> &getDeviceSupportedExtensions() { return _supportedExtensions; }
    const std::vector<VkQueueFamilyProperties> &getQueueFamilyProperties() {return _queueFamilyProperties;}

    bool isExtensionSupported(std::string_view extension) const;

    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                            VkBool32 *memTypeFound = nullptr) const;

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features) const;

private:
    VulkanInstance *m_instance = nullptr;

    VkPhysicalDeviceProperties _properties;
    VkPhysicalDeviceFeatures _features;
    VkPhysicalDeviceMemoryProperties _memoryProperties;
    std::vector<std::string> _supportedExtensions;
    std::vector<VkQueueFamilyProperties> _queueFamilyProperties;
};
}  // namespace vkl

#endif  // PHYSICALDEVICE_H_
