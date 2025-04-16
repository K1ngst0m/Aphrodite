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
auto GetFeatureEntries()
{
    static auto featureEntries = std::to_array<Instance::FeatureEntry>({
        // Validation & Debug Utils
        { .name = "Validation & Debug Utils",
         .isRequired =
              [](const InstanceFeature& features)
          {
              return features.enableValidation || features.enableDebugUtils;
          }, .setupFeature =
              [](InstanceCreateInfo& createInfo)
          {
              if (createInfo.features.enableDebugUtils)
              {
                  auto existingCallback  = createInfo.debugCreateInfo.pfnUserCallback;
                  auto* existingUserData = createInfo.debugCreateInfo.pUserData;

                  createInfo.debugCreateInfo
                      .setMessageSeverity(::vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                          ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                          ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
                      .setMessageType(::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                      ::vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                      ::vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                      ::vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding);

                  if (existingCallback)
                  {
                      createInfo.debugCreateInfo.setPfnUserCallback(existingCallback);
                  }
                  if (existingUserData)
                  {
                      createInfo.debugCreateInfo.setPUserData(existingUserData);
                  }
              }
          }, .extensionNames = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME },
         .layerNames     = { "VK_LAYER_KHRONOS_validation" },
         .isCritical     = false },
        // Surface Support
        { .name = "Window system Support",
         .isRequired =
              [](const InstanceFeature& features)
          {
              return features.enableWindowSystem;
          }, .extensionNames = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME },
         .isCritical     = true },

        // Capture Support
        { .name = "Capture Support",
         .isRequired =
              [](const InstanceFeature& features)
          {
              return features.enableCapture;
          }, .layerNames = { "VK_LAYER_LUNARG_gfxreconstruct" },
         .isCritical = false }
    });
    return featureEntries;
}
} // namespace

Instance::Instance(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo) {};

auto Instance::validateFeatures(const InstanceFeature& features, const HashSet<std::string>& supportedExtensions,
                                const HashSet<std::string>& supportedLayers,
                                const SmallVector<const char*>& enabledExtensions,
                                const SmallVector<const char*>& enabledLayers) -> Result
{
    const auto featureEntries = GetFeatureEntries();
    bool allValid             = true;

    // Validate each feature using the integrated checkFeatureSupport method
    for (const auto& entry : featureEntries)
    {
        if (!entry.checkFeatureSupport(features, supportedExtensions, supportedLayers) && entry.isCritical)
        {
            VK_LOG_ERR("Critical feature '%s' is not supported", entry.name.data());
            allValid = false;
        }
    }

    // Validate all required extensions
    SmallVector<const char*> missingExtensions;
    for (const auto& requiredExtension : enabledExtensions)
    {
        if (!supportedExtensions.contains(requiredExtension))
        {
            VK_LOG_ERR("The instance extension %s is not supported.", requiredExtension);
            missingExtensions.push_back(requiredExtension);
            allValid = false;
        }
    }

    // Validate all required layers
    SmallVector<const char*> missingLayers;
    for (const char* layerName : enabledLayers)
    {
        if (!supportedLayers.contains(layerName))
        {
            VK_LOG_ERR("The instance layer %s is not found.", layerName);
            missingLayers.push_back(layerName);
            allValid = false;
        }
    }

    // Print diagnostic information if validation fails
    if (!allValid)
    {
        if (!missingExtensions.empty())
        {
            // Print all supported extensions to help debugging
            VK_LOG_DEBUG("Supported extensions (%zu):", supportedExtensions.size());
            for (const auto& ext : supportedExtensions)
            {
                VK_LOG_DEBUG("  %s", ext.c_str());
            }
        }

        if (!missingLayers.empty())
        {
            // Print all supported layers to help debugging
            VK_LOG_INFO("Supported layers (%zu):", supportedLayers.size());
            for (const auto& layer : supportedLayers)
            {
                VK_LOG_INFO("  %s", layer.c_str());
            }
        }

        return { Result::RuntimeError,
                 "Feature validation failed: required instance features, extensions, or layers are not supported." };
    }

    return Result::Success;
}

auto Instance::setupFeatures(InstanceCreateInfo& createInfo, SmallVector<const char*>& enabledExtensions,
                             SmallVector<const char*>& enabledLayers, HashSet<std::string>& supportedExtensions,
                             HashSet<std::string>& supportedLayers) -> Result
{
    APH_PROFILER_SCOPE();
    const auto featureEntries = GetFeatureEntries();

    // 1. Enumerate supported extensions and layers
    {
        auto getSupportExtension = [&supportedExtensions](const std::string& layerName)
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
            return { Result::RuntimeError, "Failed to enumerate instance extensions" };
        }

        // Get supported layers
        auto [res, layerProperties] = ::vk::enumerateInstanceLayerProperties();
        if (res != ::vk::Result::eSuccess)
        {
            return { Result::RuntimeError, "Failed to enumerate instance layers" };
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

    // 2. First, add any explicitly required extensions and layers
    enabledExtensions.insert(enabledExtensions.end(), createInfo.explicitExtensions.begin(),
                             createInfo.explicitExtensions.end());

    enabledLayers.insert(enabledLayers.end(), createInfo.explicitLayers.begin(), createInfo.explicitLayers.end());

    // 3. Setup each required feature
    for (const auto& entry : featureEntries)
    {
        if (entry.isRequired(createInfo.features))
        {
            // Add all extensions and layers defined for this feature
            for (const auto& extension : entry.extensionNames)
            {
                enabledExtensions.push_back(extension.data());
            }

            for (const auto& layer : entry.layerNames)
            {
                enabledLayers.push_back(layer.data());
            }

            // Run any additional setup for the feature
            if (entry.setupFeature)
            {
                entry.setupFeature(createInfo);
            }
        }
    }

    // 4. Remove any duplicate extensions or layers
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

    return Result::Success;
}

auto Instance::Create(const InstanceCreateInfo& createInfo) -> Expected<Instance*>
{
    APH_PROFILER_SCOPE();

    // Create instance with minimal initialization
    auto* pInstance = new Instance(createInfo, {});
    if (pInstance == nullptr)
    {
        return { Result::RuntimeError, "Failed to allocate Instance" };
    }

    // Complete initialization
    Result initResult = pInstance->initialize(createInfo);
    if (!initResult.success())
    {
        delete pInstance;
        return { initResult.getCode(), initResult.toString() };
    }

    return pInstance;
}

auto Instance::initialize(const InstanceCreateInfo& createInfo) -> Result
{
    APH_PROFILER_SCOPE();

    // Create a mutable copy of the createInfo for modifications
    InstanceCreateInfo finalCreateInfo = createInfo;

    // Setup structure to hold our instance configurations and resources
    SmallVector<const char*> enabledExtensions;
    SmallVector<const char*> enabledLayers;
    HashSet<std::string> supportedExtensions{};
    HashSet<std::string> supportedLayers{};
    ::vk::ApplicationInfo appInfo{};
    ::vk::InstanceCreateInfo instanceCreateInfo{};
    ::vk::Instance instanceHandle{};

    //
    // 1. Collect required extensions and layers and enumerate supported ones
    //
    {
        Result setupResult =
            setupFeatures(finalCreateInfo, enabledExtensions, enabledLayers, supportedExtensions, supportedLayers);
        if (!setupResult.success())
        {
            return setupResult;
        }
    }

    //
    // 2. Validate features, extensions and layers
    //
    {
        APH_PROFILER_SCOPE();
        Result validationResult = validateFeatures(createInfo.features, supportedExtensions, supportedLayers,
                                                   enabledExtensions, enabledLayers);
        if (!validationResult.success())
        {
            return validationResult;
        }
    }

    //
    // 3. Create Vulkan instance
    //
    {
        APH_PROFILER_SCOPE();

        // Setup application info
        appInfo.setPApplicationName(finalCreateInfo.appName.c_str())
            .setPEngineName("Aphrodite")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_4);

        // Setup instance create info
        instanceCreateInfo.setPApplicationInfo(&appInfo)
            .setPEnabledLayerNames(enabledLayers)
            .setPEnabledExtensionNames(enabledExtensions);

        // Configure debug messenger if needed
#if defined(APH_DEBUG)
        if (finalCreateInfo.features.enableDebugUtils)
        {
            instanceCreateInfo.setPNext(&finalCreateInfo.debugCreateInfo);
        }
#endif

        // Create the instance
        {
            APH_PROFILER_SCOPE();
            auto [res, handle] = ::vk::createInstance(instanceCreateInfo, vk::vk_allocator());
            if (res != ::vk::Result::eSuccess)
            {
                return { Result::RuntimeError, "Failed to create Vulkan instance" };
            }

            // Store the handle and initialize the dispatcher
            m_handle = handle;
            VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);
        }
    }

    //
    // 4. Enumerate physical devices
    //
    {
        APH_PROFILER_SCOPE();
        auto [res, gpus] = m_handle.enumeratePhysicalDevices();
        if (res != ::vk::Result::eSuccess)
        {
            return { Result::RuntimeError, "Failed to enumerate physical devices" };
        }

        for (uint32_t idx = 0; const auto& gpu : gpus)
        {
            auto* physicalDevice = m_physicalDevicePools.allocate(gpu);
            auto gpuProperties   = physicalDevice->getProperties();
            VK_LOG_INFO(" == Device Info [%d] ==", idx);
            VK_LOG_INFO("Device Name: %s", gpuProperties.GpuVendorPreset.gpuName.c_str());
            VK_LOG_INFO("Driver Version: %s", gpuProperties.GpuVendorPreset.gpuDriverVersion.c_str());
            m_physicalDevices.push_back(physicalDevice);
            idx++;
        }
    }

    //
    // 5. Setup debug messenger (if in debug mode)
    //
#if defined(APH_DEBUG)
    if (finalCreateInfo.features.enableDebugUtils)
    {
        auto [res, debugMessenger] =
            m_handle.createDebugUtilsMessengerEXT(finalCreateInfo.debugCreateInfo, vk_allocator());
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

auto Instance::Destroy(Instance* pInstance) -> void
{
    if (pInstance == nullptr)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    // Clean up physical devices
    for (auto* gpu : pInstance->m_physicalDevices)
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

auto Instance::getPhysicalDevices(uint32_t idx) -> PhysicalDevice*
{
    return m_physicalDevices[idx];
}
} // namespace aph::vk
