#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "common/threadPool.h"
#include "renderer/gpuResource.h"
#include "vkUtils.h"

namespace vkl {
struct InstanceCreateInfo {
    const void              *pNext;
    const VkApplicationInfo *pApplicationInfo;
    uint32_t                 enabledLayerCount;
    const char *const       *ppEnabledLayerNames;
    uint32_t                 enabledExtensionCount;
    const char *const       *ppEnabledExtensionNames;
};

class VulkanPhysicalDevice;

class VulkanInstance : public ResourceHandle<VkInstance> {
public:
    static VkResult Create(const InstanceCreateInfo *pCreateInfo,
                           VulkanInstance          **ppInstance);

    static void Destroy(VulkanInstance *pInstance);

    const std::vector<VulkanPhysicalDevice *> &getPhysicalDevices();

    ThreadPool *GetThreadPool();

private:
    std::vector<std::string>            _validationLayers;
    std::vector<VulkanPhysicalDevice *> _physicalDevices;
    ThreadPool                         *_threadPool = nullptr;
};
} // namespace vkl

#endif // INSTANCE_H_
