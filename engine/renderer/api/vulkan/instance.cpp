#include "instance.h"
#include "physicalDevice.h"

namespace vkl {

VkResult VulkanInstance::Create(const InstanceCreateInfo *pCreateInfo,
                                VulkanInstance          **ppInstance) {
    // Fill out VkApplicationInfo struct.
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = pCreateInfo->pApplicationInfo->pNext;
    appInfo.pApplicationName   = pCreateInfo->pApplicationInfo->pApplicationName;
    appInfo.applicationVersion = pCreateInfo->pApplicationInfo->applicationVersion;
    appInfo.pEngineName        = pCreateInfo->pApplicationInfo->pEngineName;
    appInfo.engineVersion      = pCreateInfo->pApplicationInfo->engineVersion;
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    // Create VkInstance.
    VkInstanceCreateInfo instanceCreateInfo    = {};
    instanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext                   = pCreateInfo->pNext;
    instanceCreateInfo.pApplicationInfo        = &appInfo;
    instanceCreateInfo.enabledLayerCount       = pCreateInfo->enabledLayerCount;
    instanceCreateInfo.ppEnabledLayerNames     = pCreateInfo->ppEnabledLayerNames;
    instanceCreateInfo.enabledExtensionCount   = pCreateInfo->enabledExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = pCreateInfo->ppEnabledExtensionNames;

    VkInstance handle = VK_NULL_HANDLE;
    auto       result = vkCreateInstance(&instanceCreateInfo, nullptr, &handle);
    if (result != VK_SUCCESS)
        return result;

    // Create a new Instance object to wrap Vulkan handle.
    auto instance     = new VulkanInstance();
    instance->_handle = handle;

    // Get the number of attached physical devices.
    uint32_t physicalDeviceCount = 0;
    result                       = vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, nullptr);
    if (result != VK_SUCCESS)
        return result;

    // Make sure there is at least one physical device present.
    if (physicalDeviceCount > 0) {
        // Enumerate physical device handles.
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        result = vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, physicalDevices.data());
        if (result != VK_SUCCESS)
            return result;

        // Wrap native Vulkan handles in PhysicalDevice class.
        for (auto &pd : physicalDevices) {
            auto pdImpl = new VulkanPhysicalDevice(instance, pd);
            instance->_physicalDevices.push_back(pdImpl);
        }
    }

    // Get extensions supported by the instance and store for later use
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
            for (VkExtensionProperties extension : extensions) {
                instance->_supportedInstanceExtensions.push_back(extension.extensionName);
            }
        }
    }


    // Initialize the Instance's thread pool with a single worker thread for now.
    // TODO: Assess background tasks and performance before increasing thread count.
    instance->_threadPool = new ThreadPool(1);

    // Copy address of object instance.
    *ppInstance = instance;

    // Return success.
    return VK_SUCCESS;
}

ThreadPool *VulkanInstance::GetThreadPool() {
    return _threadPool;
}
VulkanPhysicalDevice * VulkanInstance::getPhysicalDevices(uint32_t idx) {
    return _physicalDevices[idx];
}
void VulkanInstance::Destroy(VulkanInstance *pInstance) {
    delete pInstance->_threadPool;
    vkDestroyInstance(pInstance->getHandle(), nullptr);
}
} // namespace vkl
