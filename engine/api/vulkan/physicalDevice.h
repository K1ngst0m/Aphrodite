#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "instance.h"
#include "api/gpuResource.h"

namespace aph
{
enum class QueueType
{
    GRAPHICS = 0,
    COMPUTE  = 1,
    TRANSFER = 2,
};

class VulkanPhysicalDevice : public ResourceHandle<VkPhysicalDevice>
{
    friend class VulkanDevice;

public:
    VulkanPhysicalDevice(VkPhysicalDevice handle);

    std::vector<uint32_t>      getQueueFamilyIndexByFlags(QueueType flags);
    bool                       isExtensionSupported(std::string_view extension) const;
    uint32_t                   findMemoryType(BufferDomain domain, uint32_t mask) const;
    uint32_t                   findMemoryType(ImageDomain domain, uint32_t mask) const;
    uint32_t                   findMemoryType(VkMemoryPropertyFlags required, uint32_t mask) const;
    VkFormat                   findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                   VkFormatFeatureFlags features) const;
    size_t                     padUniformBufferSize(size_t originalSize) const;
    VkPhysicalDeviceProperties getProperties() const { return m_properties; }

private:
    VkPhysicalDeviceProperties                           m_properties;
    VkPhysicalDeviceMemoryProperties                     m_memoryProperties;
    std::vector<std::string>                             m_supportedExtensions;
    std::vector<VkQueueFamilyProperties>                 m_queueFamilyProperties;
    std::unordered_map<QueueType, std::vector<uint32_t>> m_queueFamilyMap;
};
}  // namespace aph

#endif  // PHYSICALDEVICE_H_
