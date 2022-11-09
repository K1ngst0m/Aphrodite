#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "instance.h"

namespace vkl {
class VulkanPhysicalDevice : public ResourceHandle<VkPhysicalDevice> {
public:
    VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle);

    const VulkanInstance                       *getInstance() const;
    const VkPhysicalDeviceProperties           &getDeviceProperties();
    const VkPhysicalDeviceFeatures             &getDeviceFeatures();
    const VkPhysicalDeviceMemoryProperties     &getMemoryProperties();
    const std::vector<std::string>             &getDeviceSupportedExtensions();
    const std::vector<VkQueueFamilyProperties> &getQueueFamilyProperties();

    bool isExtensionSupported(std::string_view extension) const;

    uint32_t findMemoryType(uint32_t              typeBits,
                            VkMemoryPropertyFlags properties,
                            VkBool32             *memTypeFound = nullptr) const;

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                 VkImageTiling                tiling,
                                 VkFormatFeatureFlags         features) const;

private:
    VulkanInstance *m_instance = nullptr;

    VkPhysicalDeviceProperties           _properties;
    VkPhysicalDeviceFeatures             _features;
    VkPhysicalDeviceMemoryProperties     _memoryProperties;
    std::vector<std::string>             _supportedExtensions;
    std::vector<VkQueueFamilyProperties> _queueFamilyProperties;
};
} // namespace vkl

#endif // PHYSICALDEVICE_H_
