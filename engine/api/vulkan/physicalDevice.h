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

    bool                              isExtensionSupported(std::string_view extension) const;
    uint32_t                          findMemoryType(BufferDomain domain, uint32_t mask) const;
    uint32_t                          findMemoryType(ImageDomain domain, uint32_t mask) const;
    uint32_t                          findMemoryType(VkMemoryPropertyFlags required, uint32_t mask) const;
    VkFormat                          findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                          VkFormatFeatureFlags features) const;
    size_t                            padUniformBufferSize(size_t originalSize) const;
    const VkPhysicalDeviceProperties& getProperties() const { return m_properties; }
    const GPUSettings&                getSettings() const { return m_settings; }

    template <typename T>
    T &requestFeatures(VkStructureType type)
    {
        if (m_requestedFeatures.count(type))
        {
            return *static_cast<T *>(m_requestedFeatures.at(type).get());
        }
        VkPhysicalDeviceFeatures2KHR physicalDeviceFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR};
        T                            extension{type};
        physicalDeviceFeatures.pNext = &extension;
        vkGetPhysicalDeviceFeatures2KHR(getHandle(), &physicalDeviceFeatures);
        m_requestedFeatures.insert({type, std::make_shared<T>(extension)});
        auto *extensionPtr = static_cast<T *>(m_requestedFeatures.find(type)->second.get());
        if (m_pLastRequestedFeature)
        {
            extensionPtr->pNext = m_pLastRequestedFeature;
        }
        m_pLastRequestedFeature = extensionPtr;
        return *extensionPtr;
    }

    void* getRequestedFeatures() const {return m_pLastRequestedFeature;}

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

    void *m_pLastRequestedFeature = {};
    HashMap<VkStructureType, std::shared_ptr<void>> m_requestedFeatures;
};

}  // namespace aph::vk

namespace aph::vk::utils
{
// Determines pipeline stages involved for given accesses
VkPipelineStageFlags determinePipelineStageFlags(PhysicalDevice* pGPU, VkAccessFlags accessFlags, QueueType queueType);
}  // namespace aph::vk::utils

#endif  // PHYSICALDEVICE_H_
