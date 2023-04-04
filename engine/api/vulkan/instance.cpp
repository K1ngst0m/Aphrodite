#include "instance.h"
#include "physicalDevice.h"

namespace aph
{

#ifdef VK_CHECK_RESULT
#undef VK_CHECK_RESULT
#endif

#define VK_CHECK_RESULT(f) \
    { \
        VkResult res = (f); \
        if(res != VK_SUCCESS) \
        { \
            return res; \
        } \
    }

namespace
{

bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for(const auto &layerProperties : availableLayers)
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

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                             void *pUserData)
{
    switch(messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        std::cerr << "[DEBUG] >>> " << pCallbackData->pMessage << std::endl;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        std::cerr << "[INFO] >>> " << pCallbackData->pMessage << std::endl;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        std::cerr << "[WARNING] >>> " << pCallbackData->pMessage << std::endl;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        std::cerr << "[ERROR] >>> " << pCallbackData->pMessage << std::endl;
        break;
    default:
        break;
    }
    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

}  // namespace

VkResult VulkanInstance::Create(const InstanceCreateInfo &createInfo, VulkanInstance **ppInstance)
{
    // Fill out VkApplicationInfo struct.
    // TODO check version with supports
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = createInfo.pApplicationName,
        .pEngineName = "Aphrodite",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    // Create VkInstance.
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(createInfo.enabledLayers.size()),
        .ppEnabledLayerNames = createInfo.enabledLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(createInfo.enabledExtensions.size()),
        .ppEnabledExtensionNames = createInfo.enabledExtensions.data(),
    };

    if (createInfo.flags & INSTANCE_CREATION_ENABLE_DEBUG)
    {
        if(!checkValidationLayerSupport(createInfo.enabledLayers))
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }

    VkInstance handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &handle));

    volkLoadInstance(handle);

    // Create a new Instance object to wrap Vulkan handle.
    auto instance = new VulkanInstance();
    instance->getHandle() = handle;

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
        for(auto &pd : physicalDevices)
        {
            auto pdImpl = new VulkanPhysicalDevice(instance, pd);
            instance->m_physicalDevices.push_back(pdImpl);
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
    instance->m_threadPool = new ThreadPool(1);

    // Copy address of object instance.
    *ppInstance = instance;

    if (createInfo.flags & INSTANCE_CREATION_ENABLE_DEBUG)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);
        VK_CHECK_RESULT(createDebugUtilsMessengerEXT(handle, &createInfo, nullptr, &instance->m_debugMessenger));
    }

    // Return success.
    return VK_SUCCESS;
}

void VulkanInstance::Destroy(VulkanInstance *pInstance)
{
    delete pInstance->m_threadPool;
    destroyDebugUtilsMessengerEXT(pInstance->getHandle(), pInstance->m_debugMessenger, nullptr);
    vkDestroyInstance(pInstance->getHandle(), nullptr);
}
}  // namespace aph
