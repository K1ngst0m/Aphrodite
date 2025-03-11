#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/hash.h"
#include <vulkan/vulkan.hpp>

namespace aph::vk
{
struct InstanceCreateInfo
{
    std::string appName{ "Aphrodite" };
    SmallVector<const char*> enabledLayers{};
    SmallVector<const char*> enabledExtensions{};
    ::vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
};

class PhysicalDevice;

class Instance : public ResourceHandle<::vk::Instance, InstanceCreateInfo>
{
public:
    static Result Create(const InstanceCreateInfo& createInfo, Instance** ppInstance);
    static void Destroy(Instance* pInstance);

    PhysicalDevice* getPhysicalDevices(uint32_t idx)
    {
        return m_physicalDevices[idx];
    }

private:
#ifdef APH_DEBUG
    ::vk::DebugUtilsMessengerEXT m_debugMessenger{};
#endif
    Instance(const CreateInfoType& createInfo, HandleType handle);
    SmallVector<PhysicalDevice*> m_physicalDevices{};
    ThreadSafeObjectPool<PhysicalDevice> m_physicalDevicePools;
};
} // namespace aph::vk
