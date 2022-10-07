#ifndef PHYSICALDEVICE_H_
#define PHYSICALDEVICE_H_

#include "instance.h"

namespace vkl {
class VulkanPhysicalDevice : public ResourceHandle<VkPhysicalDevice> {
public:
    VulkanPhysicalDevice(VulkanInstance *instance, VkPhysicalDevice handle)
        : m_instance(instance){
        _handle = handle;
    }

    VulkanInstance* GetInstance() const { return m_instance; }

private:
    VulkanInstance *m_instance = nullptr;
};
} // namespace vkl

#endif // PHYSICALDEVICE_H_
