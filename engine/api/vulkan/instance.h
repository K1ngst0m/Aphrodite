#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "threads/threadPool.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{

using InstanceCreationFlags = uint32_t;
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
private:
    Instance(const InstanceCreateInfo& createInfo, VkInstance instance);

public:
    static VkResult Create(const InstanceCreateInfo& createInfo, Instance** ppInstance);
    static void     Destroy(Instance* pInstance);

    ThreadPool*     getThreadPool() { return m_threadPool.get(); }
    PhysicalDevice* getPhysicalDevices(uint32_t idx) { return m_physicalDevices[idx].get(); }

private:
#ifdef APH_DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger{};
#endif
    std::vector<const char*>                     m_supportedInstanceExtensions{};
    std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices{};
    std::unique_ptr<ThreadPool>                  m_threadPool{};
};
}  // namespace aph::vk

#endif  // INSTANCE_H_
