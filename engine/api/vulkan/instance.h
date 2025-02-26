#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <volk.h>
#include "api/gpuResource.h"
#include "common/hash.h"

namespace aph::vk
{
struct InstanceCreateInfo
{
    std::string                        appName{"Aphrodite"};
    std::vector<const char*>           enabledLayers{};
    std::vector<const char*>           enabledExtensions{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
};

class PhysicalDevice;

class Instance : public ResourceHandle<VkInstance, InstanceCreateInfo>
{
public:
    static VkResult Create(const InstanceCreateInfo& createInfo, Instance** ppInstance);
    static void     Destroy(Instance* pInstance);

    PhysicalDevice* getPhysicalDevices(uint32_t idx) { return m_physicalDevices[idx].get(); }
private:
#ifdef APH_DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger{};
#endif
    Instance(const CreateInfoType& createInfo, HandleType handle);
    HashSet<std::string>                         m_supportedExtensions{};
    std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices{};
};
}  // namespace aph::vk

#endif  // INSTANCE_H_
