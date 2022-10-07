#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "instance.h"

namespace vkl {
enum QueueFlags {
    QUEUE_TYPE_COMPUTE,
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_TRANSFER,
    QUEUE_TYPE_PRESENT,
    QUEUE_TYPE_COUNT,
};

class VulkanPhysicalDevice : public ResourceHandle<VkPhysicalDevice> {
public:
    VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle);

    const VulkanInstance                       *getInstance() const;
    const VkPhysicalDeviceProperties           &getDeviceProperties();
    const VkPhysicalDeviceFeatures             &getDeviceFeatures();
    const VkPhysicalDeviceMemoryProperties     &getMemoryProperties();
    const std::vector<std::string>             &getDeviceSupportedExtensions();
    const std::vector<VkQueueFamilyProperties> &getQueueFamilyProperties();
    VkDeviceQueueCreateInfo                     getDeviceQueueCreateInfo(QueueFlags flags);
    bool                                        extensionSupported(std::string_view extension) const;
    uint32_t                                    findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
    VkFormat                                    findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling                tiling,
                                                                    VkFormatFeatureFlags         features) const;
    uint32_t                                    getQueueFamilyIndices(QueueFlags flags);

private:
    uint32_t findQueueFamilies(VkQueueFlags queueFlags) const;

    struct {
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
        uint32_t present;
    } _queueFamilyIndices;

private:
    VulkanInstance *m_instance = nullptr;

    VkPhysicalDeviceProperties                            _properties;
    VkPhysicalDeviceFeatures                              _features;
    VkPhysicalDeviceMemoryProperties                      _memoryProperties;
    std::vector<std::string>                              _supportedExtensions;
    std::vector<VkQueueFamilyProperties>                  _queueFamilyProperties;
    std::array<VkDeviceQueueCreateInfo, QUEUE_TYPE_COUNT> _queueCreateInfos{};
};
} // namespace vkl

#endif // PHYSICALDEVICE_H_
