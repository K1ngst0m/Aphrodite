#include "instance.h"
#include "api/vulkan/vkUtils.h"
#include "common/smallVector.h"
#include "physicalDevice.h"
#include "common/logger.h"

#ifdef APH_DEBUG
namespace
{

bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    aph::SmallVector<VkLayerProperties> availableLayers(layerCount);
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

    VK_LOG_ERR("Failed to create debug messenger.");
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
Instance::Instance(const CreateInfoType& createInfo, HandleType handle) : ResourceHandle(handle, createInfo){};

VkResult Instance::Create(const InstanceCreateInfo& createInfo, Instance** ppInstance)
{
    // Get extensions supported by the instance and store for later use
    HashSet<std::string>                         supportedExtensions{};

    auto getSupportExtension = [&supportedExtensions](const char* layerName) {
        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(layerName, &extCount, nullptr);
        if(extCount > 0)
        {
            SmallVector<VkExtensionProperties> extensions(extCount);
            if(vkEnumerateInstanceExtensionProperties(layerName, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                for(VkExtensionProperties extension : extensions)
                {
                    supportedExtensions.insert(extension.extensionName);
                }
            }
        }
    };

    // vulkan implementation and implicit layers
    getSupportExtension(nullptr);
    // explicit layers
    for (const auto& layer: createInfo.enabledLayers)
    {
        getSupportExtension(layer);
    }

    bool allExtensionSupported = true;
    for (const auto& requiredExtension: createInfo.enabledExtensions)
    {
        if (!supportedExtensions.contains(requiredExtension))
        {
            VK_LOG_ERR("The instance extension %s is not supported.", requiredExtension);
            allExtensionSupported = false;
        }
    }
    if (!allExtensionSupported)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

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
    _VR(vkCreateInstance(&instanceCreateInfo, vkAllocator(), &handle));

    volkLoadInstanceOnly(handle);

    // Create a new Instance object to wrap Vulkan handle.
    auto* instance = new Instance(createInfo, handle);

    // Get the number of attached physical devices.
    uint32_t physicalDeviceCount = 0;
    _VR(vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, nullptr));

    // Make sure there is at least one physical device present.
    if(physicalDeviceCount > 0)
    {
        // Enumerate physical device handles.
        SmallVector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        _VR(vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, physicalDevices.data()));

        // Wrap native Vulkan handles in PhysicalDevice class.
        for(uint32_t idx = 0; auto& pd : physicalDevices)
        {
            auto pdImpl = std::make_unique<PhysicalDevice>(pd);
            auto gpuSettings = pdImpl->getSettings();
            VK_LOG_INFO(" == Device Info [%d] ==", idx);
            VK_LOG_INFO("Device Name: %s", gpuSettings.GpuVendorPreset.gpuName);
            VK_LOG_INFO("Driver Version: %s", gpuSettings.GpuVendorPreset.gpuDriverVersion);
            idx++;
            instance->m_physicalDevices.push_back(std::move(pdImpl));
        }
    }

    instance->m_supportedExtensions = std::move(supportedExtensions);

    // Copy address of object instance.
    *ppInstance = instance;

#if defined(APH_DEBUG)
    {
        _VR(createDebugUtilsMessengerEXT(handle, &createInfo.debugCreateInfo, vkAllocator(),
                                                     &instance->m_debugMessenger));
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
    delete pInstance;
}
}  // namespace aph::vk
