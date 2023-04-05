#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "instance.h"

namespace aph
{
enum QueueTypeBits
{
    QUEUE_GRAPHICS = 1 << 0,
    QUEUE_COMPUTE = 1 << 1,
    QUEUE_TRANSFER = 1 << 2,
};
using QueueTypeFlags = uint32_t;

class VulkanPhysicalDevice : public ResourceHandle<VkPhysicalDevice>
{
public:
    VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle);

    VulkanInstance *getInstance() const { return m_instance; }
    VkPhysicalDeviceProperties getDeviceProperties() { return m_properties; }
    VkPhysicalDeviceMemoryProperties getMemoryProperties() { return m_memoryProperties; }
    std::vector<std::string> getDeviceSupportedExtensions() { return m_supportedExtensions; }
    std::vector<VkQueueFamilyProperties> getQueueFamilyProperties() { return m_queueFamilyProperties; }
    std::vector<uint32_t> getQueueFamilyIndexByFlags(QueueTypeFlags flags)
    {
        return m_queueFamilyMap.count(flags) ? m_queueFamilyMap[flags] : std::vector<uint32_t>();
    }

    bool isExtensionSupported(std::string_view extension) const;

    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                            VkBool32 *memTypeFound = nullptr) const;

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features) const;

private:
    VulkanInstance *m_instance {};

    VkPhysicalDeviceProperties m_properties;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;
    std::vector<std::string> m_supportedExtensions;
    std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
    std::unordered_map<QueueTypeFlags, std::vector<uint32_t>> m_queueFamilyMap;
};
}  // namespace aph

#endif  // PHYSICALDEVICE_H_
