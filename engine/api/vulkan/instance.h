#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "common/threadPool.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{

using InstanceCreationFlags = uint32_t;

struct InstanceCreateInfo
{
    const char*              pApplicationName = "Aphrodite";
    std::vector<const char*> enabledLayers{};
    std::vector<const char*> enabledExtensions{};
};

class PhysicalDevice;

class Instance : public ResourceHandle<VkInstance>
{
private:
    Instance() = default;

public:
    static VkResult Create(const InstanceCreateInfo& createInfo, Instance** ppInstance);

    static void Destroy(Instance* pInstance);

    ThreadPool*     getThreadPool() { return m_threadPool; }
    PhysicalDevice* getPhysicalDevices(uint32_t idx) { return m_physicalDevices[idx].get(); }

private:
    VkDebugUtilsMessengerEXT                     m_debugMessenger{};
    std::vector<const char*>                     m_supportedInstanceExtensions{};
    std::vector<std::string>                     m_validationLayers{};
    std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices{};
    ThreadPool*                                  m_threadPool{};
};
}  // namespace aph::vk

#endif  // INSTANCE_H_
