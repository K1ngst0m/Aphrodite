#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "common/hash.h"
#include "forward.h"
#include "vkUtils.h"
#include <vulkan/vulkan.hpp>

namespace aph::vk
{
/**
 * @brief Structure representing instance features
 */
struct InstanceFeature
{
    // Validation and debugging
    bool enableValidation : 1 = false;
    bool enableDebugUtils : 1 = false;

    // Window system interaction
    bool enableSurface : 1 = true;
    bool enableSurfaceCapabilities : 1 = true;

    // Physical device features
    bool enablePhysicalDeviceProperties2 : 1 = true;

    // Debug/Profiling tools
    bool enableCapture : 1 = false;
};

/**
 * @brief Structure for configuring instance creation
 */
struct InstanceCreateInfo
{
    std::string appName{"Aphrodite"};
    InstanceFeature features{};

    // Advanced usage - explicit extensions and layers
    // These are normally managed automatically based on features
    SmallVector<const char*> explicitLayers{};
    SmallVector<const char*> explicitExtensions{};

    // Debug messenger config - only used when enableDebugUtils is true
    ::vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
};

class Instance : public ResourceHandle<::vk::Instance, InstanceCreateInfo>
{
public:
    /**
     * @brief Structure representing a single instance feature/extension requirement
     */
    struct FeatureEntry
    {
        // Feature identifier for debugging and error messages
        std::string_view name;

        // Function to check if the feature is required based on InstanceFeature flags
        std::function<bool(const InstanceFeature&)> isRequired;

        // Function to check if the feature is supported
        std::function<bool(const HashSet<std::string>&, const HashSet<std::string>&)> isSupported;

        // Function to setup the feature in Vulkan's structure chain or modify CreateInfo
        std::function<void(InstanceCreateInfo&, SmallVector<const char*>&, SmallVector<const char*>&)> setupFeature;

        // Extensions related to this feature
        SmallVector<std::string_view> extensionNames;

        // Layers related to this feature
        SmallVector<std::string_view> layerNames;

        // Is this feature critical (will cause application to fail if not supported)
        bool isCritical = true;
    };

private:
    Instance(const CreateInfoType& createInfo, HandleType handle);
    Result initialize(const InstanceCreateInfo& createInfo);
    ~Instance() = default;

public:
    // Factory methods
    static Expected<Instance*> Create(const InstanceCreateInfo& createInfo);
    static void Destroy(Instance* pInstance);

    PhysicalDevice* getPhysicalDevices(uint32_t idx)
    {
        return m_physicalDevices[idx];
    }

private:
    /**
     * @brief Validate all required features against supported features
     * 
     * @param features The required instance features
     * @param supportedExtensions All supported extensions
     * @param supportedLayers All supported layers
     * @return true if all required features are supported
     */
    static bool validateFeatures(const InstanceFeature& features, const HashSet<std::string>& supportedExtensions,
                                 const HashSet<std::string>& supportedLayers);

    /**
     * @brief Setup required extensions and layers based on feature requirements
     * 
     * @param createInfo Creation info with required features
     * @param enabledExtensions Output vector to populate with required extension names
     * @param enabledLayers Output vector to populate with required layer names
     */
    static void setupRequiredFeaturesAndExtensions(const InstanceCreateInfo& createInfo,
                                                   SmallVector<const char*>& enabledExtensions,
                                                   SmallVector<const char*>& enabledLayers);

#ifdef APH_DEBUG
    ::vk::DebugUtilsMessengerEXT m_debugMessenger{};
#endif
    SmallVector<PhysicalDevice*> m_physicalDevices{};
    ThreadSafeObjectPool<PhysicalDevice> m_physicalDevicePools;
};
} // namespace aph::vk
