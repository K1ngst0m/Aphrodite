#include "instance.h"
#include "physicalDevice.h"
#include "common/logger.h"

#ifdef APH_DEBUG
namespace
{

bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers)
        {
            if(strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if(!layerFound)
        {
            return false;
        }
    }

    return true;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT*    pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

}  // namespace
#endif

namespace aph::vk
{

#ifdef VK_CHECK_RESULT
#    undef VK_CHECK_RESULT
#endif

#define VK_CHECK_RESULT(f) \
    { \
        VkResult res = (f); \
        if(res != VK_SUCCESS) \
        { \
            return res; \
        } \
    }

Instance::Instance(const CreateInfoType& createInfo, HandleType handle):
    ResourceHandle(handle, createInfo)
{
};

VkResult Instance::Create(const InstanceCreateInfo& createInfo, Instance** ppInstance)
{
    // Fill out VkApplicationInfo struct.
    // TODO check version with supports
    VkApplicationInfo appInfo = {
        .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = createInfo.appName.c_str(),
        .pEngineName      = "Aphrodite",
        .engineVersion    = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion       = VK_API_VERSION_1_3,
    };

    // Create VkInstance.
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(createInfo.enabledLayers.size()),
        .ppEnabledLayerNames     = createInfo.enabledLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(createInfo.enabledExtensions.size()),
        .ppEnabledExtensionNames = createInfo.enabledExtensions.data(),
    };

#if defined(APH_DEBUG)
    if(!checkValidationLayerSupport(createInfo.enabledLayers))
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&createInfo.debugCreateInfo;
#endif

    VkInstance handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, vkAllocator(), &handle));

    volkLoadInstance(handle);

    // Create a new Instance object to wrap Vulkan handle.
    auto* instance = new Instance(createInfo, handle);

    // Get the number of attached physical devices.
    uint32_t physicalDeviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, nullptr));

    // Make sure there is at least one physical device present.
    if(physicalDeviceCount > 0)
    {
        // Enumerate physical device handles.
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, physicalDevices.data()));

        // Wrap native Vulkan handles in PhysicalDevice class.
        for(uint32_t idx = 0; auto& pd : physicalDevices)
        {
            auto pdImpl = std::make_unique<PhysicalDevice>(pd);
        #if APH_DEBUG
            auto gpuSettings = pdImpl->getSettings();
            VK_LOG_INFO(" == Device Info [%d] ==", idx);
            VK_LOG_INFO("Device Name: %s", gpuSettings.GpuVendorPreset.gpuName);
            VK_LOG_INFO("Driver Version: %s", gpuSettings.GpuVendorPreset.gpuDriverVersion);
        #endif
            idx++;
            instance->m_physicalDevices.push_back(std::move(pdImpl));
        }
    }

    // Get extensions supported by the instance and store for later use
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if(extCount > 0)
    {
        std::vector<VkExtensionProperties> extensions(extCount);
        if(vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
        {
            for(VkExtensionProperties extension : extensions)
            {
                instance->m_supportedInstanceExtensions.push_back(extension.extensionName);
            }
        }
    }

    // Initialize the Instance's thread pool with a single worker thread for now.
    // TODO: Assess background tasks and performance before increasing thread count.
    instance->m_threadPool = std::make_unique<ThreadPool>(1);

    // Copy address of object instance.
    *ppInstance = instance;

#if defined(APH_DEBUG)
    {
        VK_CHECK_RESULT(
            createDebugUtilsMessengerEXT(handle, &createInfo.debugCreateInfo, vkAllocator(), &instance->m_debugMessenger));
    }
#endif
    // Return success.
    return VK_SUCCESS;
}

void Instance::Destroy(Instance* pInstance)
{
#ifdef APH_DEBUG
    destroyDebugUtilsMessengerEXT(pInstance->getHandle(), pInstance->m_debugMessenger, vkAllocator());
#endif
    vkDestroyInstance(pInstance->getHandle(), vkAllocator());
}
}  // namespace aph::vk
