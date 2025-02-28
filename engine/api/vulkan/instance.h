#pragma once

#include "api/gpuResource.h"
#include "common/hash.h"
#include <vulkan/vulkan.hpp>

namespace aph::vk
{
struct InstanceCreateInfo
{
    std::string appName{ "Aphrodite" };
    std::vector<const char*> enabledLayers{};
    std::vector<const char*> enabledExtensions{};
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
        return m_physicalDevices[idx].get();
    }

private:
#ifdef APH_DEBUG
    ::vk::DebugUtilsMessengerEXT m_debugMessenger{};
#endif
    Instance(const CreateInfoType& createInfo, HandleType handle);
    std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices{};
};
} // namespace aph::vk
