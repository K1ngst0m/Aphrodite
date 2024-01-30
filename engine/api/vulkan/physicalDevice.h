#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "common/hash.h"
#include "instance.h"

namespace aph::vk
{

class PhysicalDevice : public ResourceHandle<VkPhysicalDevice>
{
    friend class Device;

public:
    PhysicalDevice(HandleType handle);

    const std::vector<uint32_t>&      getQueueFamilyIndexByFlags(QueueType flags);
    bool                              isExtensionSupported(std::string_view extension) const;
    uint32_t                          findMemoryType(BufferDomain domain, uint32_t mask) const;
    uint32_t                          findMemoryType(ImageDomain domain, uint32_t mask) const;
    uint32_t                          findMemoryType(VkMemoryPropertyFlags required, uint32_t mask) const;
    VkFormat                          findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                          VkFormatFeatureFlags features) const;
    size_t                            padUniformBufferSize(size_t originalSize) const;
    const VkPhysicalDeviceProperties& getProperties() const { return m_properties; }
    const GPUSettings&                getSettings() const { return m_settings; }

private:
    GPUSettings                               m_settings              = {};
    VkPhysicalDeviceDriverProperties          m_driverProperties      = {};
    VkPhysicalDeviceProperties                m_properties            = {};
    VkPhysicalDeviceProperties2               m_properties2           = {};
    VkPhysicalDeviceFeatures                  m_features              = {};
    VkPhysicalDeviceFeatures2                 m_features2             = {};
    VkPhysicalDeviceMemoryProperties          m_memoryProperties      = {};
    std::vector<std::string>                  m_supportedExtensions   = {};
    std::vector<VkQueueFamilyProperties>      m_queueFamilyProperties = {};
    HashMap<QueueType, std::vector<uint32_t>> m_queueFamilyMap        = {};
};

}  // namespace aph::vk

namespace aph::vk::utils
{
// Determines pipeline stages involved for given accesses
VkPipelineStageFlags determinePipelineStageFlags(PhysicalDevice* pGPU, VkAccessFlags accessFlags, QueueType queueType);
}  // namespace aph::vk::utils

#endif  // PHYSICALDEVICE_H_
