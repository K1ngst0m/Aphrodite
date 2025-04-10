#include "instance.h"
#include "physicalDevice.h"

#include "common/logger.h"
#include "common/profiler.h"
#include "exception/errorMacros.h"

#include "api/vulkan/vkUtils.h"

namespace aph::vk
{
namespace
{
// Define the feature entries with all validation and setup logic
auto getFeatureEntries()
{
    static auto
        featureEntries = std::to_array<Instance::FeatureEntry>({{.name = "Surface Support",
                                                                 .isRequired = [](const InstanceFeature& features)
                                                                 { return features.enableSurface; },
                                                                 .isSupported =
                                                                     [](const HashSet<std::string>& supportedExtensions,
                                                                        const HashSet<std::string>& supportedLayers)
                                                                 {
                                                                     return supportedExtensions.contains(
                                                                         VK_KHR_SURFACE_EXTENSION_NAME);
                                                                 },
                                                                 .setupFeature =
                                                                     [](InstanceCreateInfo& createInfo,
                                                                        SmallVector<const char*>& enabledExtensions,
                                                                        SmallVector<const char*>& enabledLayers)
                                                                 {
                                                                     // Setup any core required extensions - these are required for any Vulkan application
                                                                     enabledExtensions.push_back(
                                                                         VK_KHR_SURFACE_EXTENSION_NAME);
                                                                 },
                                                                 .extensionNames = {VK_KHR_SURFACE_EXTENSION_NAME},
                                                                 .layerNames = {},
                                                                 .isCritical = true},
                                                                {.name = "Validation & Debug Utils",
                                                                 .isRequired =
                                                                     [](const InstanceFeature& features)
                                                                 {
                                                                     return features.enableValidation ||
                                                                            features.enableDebugUtils;
                                                                 },
                                                                 .isSupported =
                                                                     [](const HashSet<std::string>& supportedExtensions,
                                                                        const HashSet<std::string>& supportedLayers)
                                                                 {
                                                                     bool validation = supportedLayers.contains(
                                                                         "VK_LAYER_KHRONOS_validation");
                                                                     bool debugUtils = supportedExtensions.contains(
                                                                         VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

                                                                     return validation && debugUtils;
                                                                 },
                                                                 .setupFeature =
                                                                     [](
                                                                         InstanceCreateInfo& createInfo,
                                                                         SmallVector<const char*>& enabledExtensions,
                                                                         SmallVector<const char*>& enabledLayers)
                                                                 {
                                                                     if (createInfo.features.enableValidation)
                                                                     {
                                                                         // Add validation layer
                                                                         enabledLayers.push_back(
                                                                             "VK_LAYER_KHRONOS_validation");
                                                                     }

                                                                     if (createInfo.features.enableDebugUtils)
                                                                     {
                                                                         // Add debug utils extension - must have this extension if using debug messenger
                                                                         enabledExtensions.push_back(
                                                                             VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

                                                                         // Setup debug messenger if not already configured
                                                                         if (createInfo.debugCreateInfo
                                                                                 .messageSeverity ==
                                                                             static_cast<
                                                                                 ::vk::
                                                                                     DebugUtilsMessageSeverityFlagsEXT>(
                                                                                 0))
                                                                         {
                                                                             // Save the existing callback function and user data
                                                                             auto existingCallback =
                                                                                 createInfo.debugCreateInfo
                                                                                     .pfnUserCallback;
                                                                             auto existingUserData =
                                                                                 createInfo.debugCreateInfo.pUserData;

                                                                             // Configure the messenger
                                                                             createInfo.debugCreateInfo
                                                                                 .setMessageSeverity(
                                                                                     ::vk::
                                                                                         DebugUtilsMessageSeverityFlagBitsEXT::
                                                                                             eError |
                                                                                     ::vk::
                                                                                         DebugUtilsMessageSeverityFlagBitsEXT::
                                                                                             eWarning |
                                                                                     ::vk::
                                                                                         DebugUtilsMessageSeverityFlagBitsEXT::
                                                                                             eInfo)
                                                                                 .setMessageType(
                                                                                     ::vk::
                                                                                         DebugUtilsMessageTypeFlagBitsEXT::
                                                                                             eGeneral |
                                                                                     ::vk::
                                                                                         DebugUtilsMessageTypeFlagBitsEXT::
                                                                                             ePerformance |
                                                                                     ::vk::
                                                                                         DebugUtilsMessageTypeFlagBitsEXT::
                                                                                             eValidation |
                                                                                     ::vk::
                                                                                         DebugUtilsMessageTypeFlagBitsEXT::
                                                                                             eDeviceAddressBinding);

                                                                             // Restore the callback and user data if they were set
                                                                             if (existingCallback)
                                                                             {
                                                                                 createInfo.debugCreateInfo
                                                                                     .setPfnUserCallback(
                                                                                         existingCallback);
                                                                             }

                                                                             if (existingUserData)
                                                                             {
                                                                                 createInfo.debugCreateInfo
                                                                                     .setPUserData(existingUserData);
                                                                             }
                                                                         }
                                                                     }
                                                                 },
                                                                 .extensionNames = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME},
                                                                 .layerNames = {"VK_LAYER_KHRONOS_validation"},
                                                                 .isCritical = false},
                                                                {.name = "Physical Device Properties 2",
                                                                 .isRequired = [](const InstanceFeature& features)
                                                                 { return features.enablePhysicalDeviceProperties2; },
                                                                 .isSupported =
                                                                     [](const HashSet<std::string>& supportedExtensions,
                                                                        const HashSet<std::string>& supportedLayers)
                                                                 {
                                                                     return supportedExtensions.contains(
                                                                         VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                                                                 },
                                                                 .setupFeature =
                                                                     [](InstanceCreateInfo& createInfo,
                                                                        SmallVector<const char*>& enabledExtensions,
                                                                        SmallVector<const char*>& enabledLayers)
                                                                 {
                                                                     // Required for advanced device features like mesh shading, ray tracing, etc.
                                                                     enabledExtensions.push_back(
                                                                         VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                                                                 },
                                                                 .extensionNames =
                                                                     {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
                                                                 .layerNames = {},
                                                                 .isCritical = true},
                                                                {.name = "Surface Capabilities",
                                                                 .isRequired = [](const InstanceFeature& features)
                                                                 { return features.enableSurfaceCapabilities; },
                                                                 .isSupported =
                                                                     [](const HashSet<std::string>& supportedExtensions,
                                                                        const HashSet<std::string>& supportedLayers)
                                                                 {
                                                                     return supportedExtensions.contains(
                                                                         VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                                                                 },
                                                                 .setupFeature =
                                                                     [](InstanceCreateInfo& createInfo,
                                                                        SmallVector<const char*>& enabledExtensions,
                                                                        SmallVector<const char*>& enabledLayers)
                                                                 {
                                                                     // Required for advanced surface capabilities like HDR, etc.
                                                                     enabledExtensions.push_back(
                                                                         VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                                                                 },
                                                                 .extensionNames =
                                                                     {VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME},
                                                                 .layerNames = {},
                                                                 .isCritical = false},
                                                                {.name = "Capture Support",
                                                                 .isRequired = [](const InstanceFeature& features)
                                                                 { return features.enableCapture; },
                                                                 .isSupported =
                                                                     [](const HashSet<std::string>& supportedExtensions,
                                                                        const HashSet<std::string>& supportedLayers)
                                                                 { return true; },
                                                                 .setupFeature =
                                                                     [](InstanceCreateInfo& createInfo,
                                                                        SmallVector<const char*>& enabledExtensions,
                                                                        SmallVector<const char*>& enabledLayers)
                                                                 {
                                                                     // If you need any special extensions or layers for capture tools like RenderDoc
                                                                     // they would be added here
                                                                 },
                                                                 .extensionNames = {},
                                                                 .layerNames = {},
                                                                 .isCritical = false}});
    return featureEntries;
}
} // namespace

Instance::Instance(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo) {};

bool Instance::validateFeatures(const InstanceFeature& features, const HashSet<std::string>& supportedExtensions,
                                const HashSet<std::string>& supportedLayers)
{
    bool allSupported         = true;
    const auto featureEntries = getFeatureEntries();

    // Validate each feature
    for (const auto& entry : featureEntries)
    {
        if (entry.isRequired(features) && !entry.isSupported(supportedExtensions, supportedLayers))
        {
            VK_LOG_ERR("%s feature not supported but required!", entry.name.data());

            if (entry.isCritical)
            {
                APH_ASSERT(false);
                allSupported = false;
            }
            else
            {
                VK_LOG_WARN("%s feature not supported but not critical - continuing anyway", entry.name.data());
            }
        }
    }

    return allSupported;
}

void Instance::setupRequiredFeaturesAndExtensions(const InstanceCreateInfo& createInfo,
                                                  SmallVector<const char*>& enabledExtensions,
                                                  SmallVector<const char*>& enabledLayers)
{
    const auto featureEntries = getFeatureEntries();
    InstanceCreateInfo* modifiableCreateInfo =
        const_cast<InstanceCreateInfo*>(&createInfo); // Use direct modification to preserve debug messenger

    // First, add any explicitly required extensions and layers
    enabledExtensions.insert(enabledExtensions.end(), createInfo.explicitExtensions.begin(),
                             createInfo.explicitExtensions.end());

    enabledLayers.insert(enabledLayers.end(), createInfo.explicitLayers.begin(), createInfo.explicitLayers.end());

    // Setup each required feature
    for (const auto& entry : featureEntries)
    {
        if (entry.isRequired(createInfo.features))
        {
            entry.setupFeature(*modifiableCreateInfo, enabledExtensions, enabledLayers);
        }
    }

    // Remove any duplicate extensions or layers
    {
        HashSet<std::string_view> uniqueExtensions;
        SmallVector<const char*> filteredExtensions;

        for (const auto& extension : enabledExtensions)
        {
            if (uniqueExtensions.insert(extension).second)
            {
                filteredExtensions.push_back(extension);
            }
        }

        enabledExtensions = std::move(filteredExtensions);
    }

    {
        HashSet<std::string_view> uniqueLayers;
        SmallVector<const char*> filteredLayers;

        for (const auto& layer : enabledLayers)
        {
            if (uniqueLayers.insert(layer).second)
            {
                filteredLayers.push_back(layer);
            }
        }

        enabledLayers = std::move(filteredLayers);
    }
}

Expected<Instance*> Instance::Create(const InstanceCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Create instance with minimal initialization
    auto* pInstance = new Instance(createInfo, {});
    if (!pInstance)
    {
        return {Result::RuntimeError, "Failed to allocate Instance"};
    }

    // Complete initialization
    Result initResult = pInstance->initialize(createInfo);
    if (!initResult.success())
    {
        delete pInstance;
        return {initResult.getCode(), initResult.toString()};
    }

    return pInstance;
}

Result Instance::initialize(const InstanceCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Setup structure to hold our instance configurations and resources
    SmallVector<const char*> enabledExtensions;
    SmallVector<const char*> enabledLayers;
    HashSet<std::string> supportedExtensions{};
    HashSet<std::string> supportedLayers{};
    ::vk::ApplicationInfo appInfo{};
    ::vk::InstanceCreateInfo instanceCreateInfo{};
    ::vk::Instance instanceHandle{};

    //
    // 1. Collect required extensions and layers
    //
    {
        setupRequiredFeaturesAndExtensions(createInfo, enabledExtensions, enabledLayers);
    }

    //
    // 2. Enumerate supported extensions and layers
    //
    {
        APH_PROFILER_SCOPE();

        auto getSupportExtension = [&supportedExtensions](std::string layerName)
        {
            auto [res, extensions] = ::vk::enumerateInstanceExtensionProperties(layerName);
            if (res != ::vk::Result::eSuccess)
            {
                return false;
            }
            for (VkExtensionProperties extension : extensions)
            {
                supportedExtensions.insert(extension.extensionName);
            }
            return true;
        };

        // Vulkan implementation and implicit layers
        if (!getSupportExtension(""))
        {
            return {Result::RuntimeError, "Failed to enumerate instance extensions"};
        }

        // Get supported layers
        auto [res, layerProperties] = ::vk::enumerateInstanceLayerProperties();
        if (res != ::vk::Result::eSuccess)
        {
            return {Result::RuntimeError, "Failed to enumerate instance layers"};
        }

        for (const auto& layerProperty : layerProperties)
        {
            supportedLayers.insert(layerProperty.layerName);

            // Get extensions provided by explicit layers
            if (!getSupportExtension(layerProperty.layerName))
            {
                VK_LOG_WARN("Failed to enumerate extensions for layer: %s", layerProperty.layerName);
            }
        }
    }

    //
    // 3. Validate feature support
    //
    {
        if (!validateFeatures(createInfo.features, supportedExtensions, supportedLayers))
        {
            return {Result::RuntimeError, "Not all required features are supported"};
        }
    }

    //
    // 4. Validate extension support
    //
    {
        APH_PROFILER_SCOPE();

        bool allExtensionSupported = true;
        for (const auto& requiredExtension : enabledExtensions)
        {
            if (!supportedExtensions.contains(requiredExtension))
            {
                VK_LOG_ERR("The instance extension %s is not supported.", requiredExtension);
                allExtensionSupported = false;
            }
        }
        if (!allExtensionSupported)
        {
            return {Result::RuntimeError, "Required instance extensions are not fully supported."};
        }
    }

    //
    // 5. Validate layer support
    //
    {
        APH_PROFILER_SCOPE();

        bool allLayersFound = true;
        for (const char* layerName : enabledLayers)
        {
            if (!supportedLayers.contains(layerName))
            {
                VK_LOG_ERR("The instance layer %s is not found.", layerName);
                allLayersFound = false;
            }
        }
        if (!allLayersFound)
        {
            return {Result::RuntimeError, "Required instance layers are not found."};
        }
    }

    //
    // 6. Create Vulkan instance
    //
    {
        APH_PROFILER_SCOPE();

        // Setup application info
        appInfo.setPApplicationName(createInfo.appName.c_str())
            .setPEngineName("Aphrodite")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_4);

        // Setup instance create info
        instanceCreateInfo.setPApplicationInfo(&appInfo)
            .setPEnabledLayerNames(enabledLayers)
            .setPEnabledExtensionNames(enabledExtensions);

        // Configure debug messenger if needed
#if defined(APH_DEBUG)
        if (createInfo.features.enableDebugUtils)
        {
            // Double-check that the extension is actually in our list before setting pNext
            bool debugUtilsExtensionEnabled = false;
            for (const auto& ext : enabledExtensions)
            {
                if (std::string_view(ext) == VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
                {
                    debugUtilsExtensionEnabled = true;
                    break;
                }
            }

            if (debugUtilsExtensionEnabled)
            {
                instanceCreateInfo.setPNext(&createInfo.debugCreateInfo);
            }
            else
            {
                VK_LOG_WARN("Debug messenger requested but VK_EXT_DEBUG_UTILS_EXTENSION_NAME not enabled or available");
            }
        }
#endif

        // Create the instance
        {
            APH_PROFILER_SCOPE();
            auto [res, handle] = ::vk::createInstance(instanceCreateInfo, vk::vk_allocator());
            if (res != ::vk::Result::eSuccess)
            {
                return {Result::RuntimeError, "Failed to create Vulkan instance"};
            }

            // Store the handle and initialize the dispatcher
            m_handle = handle;
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);
        }
    }

    //
    // 7. Enumerate physical devices
    //
    {
        APH_PROFILER_SCOPE();
        auto [res, gpus] = m_handle.enumeratePhysicalDevices();
        if (res != ::vk::Result::eSuccess)
        {
            return {Result::RuntimeError, "Failed to enumerate physical devices"};
        }

        for (uint32_t idx = 0; const auto& gpu : gpus)
        {
            auto pdImpl        = m_physicalDevicePools.allocate(gpu);
            auto gpuProperties = pdImpl->getProperties();
            VK_LOG_INFO(" == Device Info [%d] ==", idx);
            VK_LOG_INFO("Device Name: %s", gpuProperties.GpuVendorPreset.gpuName.c_str());
            VK_LOG_INFO("Driver Version: %s", gpuProperties.GpuVendorPreset.gpuDriverVersion.c_str());
            m_physicalDevices.push_back(std::move(pdImpl));
            idx++;
        }
    }

    //
    // 8. Setup debug messenger (if in debug mode)
    //
#if defined(APH_DEBUG)
    if (createInfo.features.enableDebugUtils)
    {
        // Make a local copy of the debug create info to avoid const issues
        auto debugCreateInfo = createInfo.debugCreateInfo;

        auto [res, debugMessenger] = m_handle.createDebugUtilsMessengerEXT(debugCreateInfo, vk_allocator());
        VK_VR(res);
        if (res != ::vk::Result::eSuccess)
        {
            VK_LOG_WARN("Failed to create debug messenger.");
            // Non-fatal error, just log warning
        }
        else
        {
            m_debugMessenger = debugMessenger;
        }
    }
#endif

    return Result::Success;
}

void Instance::Destroy(Instance* pInstance)
{
    if (!pInstance)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    // Clean up physical devices
    for (auto gpu : pInstance->m_physicalDevices)
    {
        pInstance->m_physicalDevicePools.free(gpu);
    }
    pInstance->m_physicalDevicePools.clear();

    // Clean up debug messenger
#ifdef APH_DEBUG
    if (pInstance->m_debugMessenger)
    {
        pInstance->m_handle.destroyDebugUtilsMessengerEXT(pInstance->m_debugMessenger, vk_allocator());
    }
#endif

    // Destroy the instance
    if (pInstance->m_handle)
    {
        pInstance->m_handle.destroy(vk_allocator());
    }

    // Delete the instance
    delete pInstance;
}
} // namespace aph::vk
