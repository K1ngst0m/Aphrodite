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
    bool enableWindowSystem : 1 = true;

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
    std::string appName{ "Aphrodite" };
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
        std::function<bool(const HashSet<std::string>&, const HashSet<std::string>&)> isSupported = nullptr;

        // Function to setup the feature in Vulkan's structure chain or modify CreateInfo
        // Takes a mutable reference to InstanceCreateInfo to allow feature-specific configuration
        std::function<void(InstanceCreateInfo&)> setupFeature = nullptr;

        // Extensions related to this feature
        SmallVector<std::string_view> extensionNames = {};

        // Layers related to this feature
        SmallVector<std::string_view> layerNames = {};

        // Is this feature critical (will cause application to fail if not supported)
        bool isCritical = true;

        /**
         * @brief Check if the feature is supported and report missing extensions and layers
         * @param features The InstanceFeature flags
         * @param supportedExtensions Set of supported extensions
         * @param supportedLayers Set of supported layers
         * @return true if the feature is either not required or is supported, false otherwise
         */
        auto checkFeatureSupport(const InstanceFeature& features, const HashSet<std::string>& supportedExtensions,
                                 const HashSet<std::string>& supportedLayers) const -> bool
        {
            // If feature is not required, it's considered "supported"
            if (!isRequired(features))
            {
                return true;
            }

            // Use either the custom isSupported function or the default implementation
            if (isSupported)
            {
                // Use custom implementation which handles its own reporting
                bool supported = isSupported(supportedExtensions, supportedLayers);

                // Only add the critical warning if not supported and not critical
                if (!supported && !isCritical)
                {
                    VK_LOG_WARN("%s feature not supported but not critical - continuing anyway", name.data());
                    return true;
                }

                return supported;
            }

            // Use default implementation with standard reporting
            bool supported = defaultIsSupported(supportedExtensions, supportedLayers);

            // If not supported, report missing components
            if (!supported)
            {
                VK_LOG_ERR("%s feature not supported but required!", name.data());

                // Standard reporting format for required components
                reportRequiredComponents();

                // Standard reporting format for missing components
                SmallVector<std::string> missingExtensions = findMissingExtensions(supportedExtensions);
                SmallVector<std::string> missingLayers     = findMissingLayers(supportedLayers);

                bool hasMissingComponents = !missingExtensions.empty() || !missingLayers.empty();

                if (hasMissingComponents)
                {
                    if (!missingExtensions.empty())
                    {
                        VK_LOG_ERR("  Missing extensions:");
                        for (const auto& ext : missingExtensions)
                        {
                            VK_LOG_ERR("    - %s", ext.c_str());
                        }
                    }

                    if (!missingLayers.empty())
                    {
                        VK_LOG_ERR("  Missing layers:");
                        for (const auto& layer : missingLayers)
                        {
                            VK_LOG_ERR("    - %s", layer.c_str());
                        }
                    }
                }

                if (!isCritical)
                {
                    VK_LOG_WARN("%s feature not supported but not critical - continuing anyway", name.data());
                    return true;
                }

                return false;
            }

            return true;
        }

    private:
        /**
         * @brief Default implementation to check if all required extensions and layers are supported
         * @param supportedExtensions Set of supported extensions
         * @param supportedLayers Set of supported layers
         * @return true if all extensions and layers are supported, false otherwise
         */
        auto defaultIsSupported(const HashSet<std::string>& supportedExtensions,
                                const HashSet<std::string>& supportedLayers) const -> bool
        {
            // First check all required extensions
            for (const auto& extName : extensionNames)
            {
                std::string extNameStr(extName);
                if (!supportedExtensions.contains(extNameStr))
                {
                    return false;
                }
            }

            return std::ranges::all_of(layerNames,
                                       [&supportedLayers](const auto& layerName)
                                       {
                                           return supportedLayers.contains(std::string(layerName));
                                       });
        }

        /**
         * @brief Report all required extensions and layers for this feature
         */
        void reportRequiredComponents() const
        {
            if (!extensionNames.empty())
            {
                VK_LOG_INFO("  Required extensions for %s:", name.data());
                for (const auto& ext : extensionNames)
                {
                    VK_LOG_INFO("    - %s", ext.data());
                }
            }

            if (!layerNames.empty())
            {
                VK_LOG_INFO("  Required layers for %s:", name.data());
                for (const auto& layer : layerNames)
                {
                    VK_LOG_INFO("    - %s", layer.data());
                }
            }
        }

        /**
         * @brief Find missing extensions
         * @param supportedExtensions Set of supported extensions
         * @return Vector of missing extension names
         */
        auto findMissingExtensions(const HashSet<std::string>& supportedExtensions) const -> SmallVector<std::string>
        {
            SmallVector<std::string> missing;

            for (const auto& extName : extensionNames)
            {
                std::string extNameStr(extName);
                if (!supportedExtensions.contains(extNameStr))
                {
                    missing.push_back(extNameStr);
                }
            }

            return missing;
        }

        /**
         * @brief Find missing layers
         * @param supportedLayers Set of supported layers
         * @return Vector of missing layer names
         */
        auto findMissingLayers(const HashSet<std::string>& supportedLayers) const -> SmallVector<std::string>
        {
            SmallVector<std::string> missing;

            for (const auto& layerName : layerNames)
            {
                std::string layerNameStr(layerName);
                if (supportedLayers.contains(layerNameStr))
                {
                    missing.push_back(layerNameStr);
                }
            }

            return missing;
        }
    };

private:
    Instance(const CreateInfoType& createInfo, HandleType handle);
    auto initialize(const InstanceCreateInfo& createInfo) -> Result;
    ~Instance() = default;

public:
    // Factory methods
    static auto Create(const InstanceCreateInfo& createInfo) -> Expected<Instance*>;
    static auto Destroy(Instance* pInstance) -> void;

    auto getPhysicalDevices(uint32_t idx) -> PhysicalDevice*;

private:
    /**
     * @brief Validate instance features, extensions and layers
     * 
     * @param features Instance features to validate
     * @param supportedExtensions Available extensions
     * @param supportedLayers Available layers
     * @param enabledExtensions Extensions to be enabled
     * @param enabledLayers Layers to be enabled
     * @return Result with validation status
     */
    static auto validateFeatures(const InstanceFeature& features, const HashSet<std::string>& supportedExtensions,
                                 const HashSet<std::string>& supportedLayers,
                                 const SmallVector<const char*>& enabledExtensions = {},
                                 const SmallVector<const char*>& enabledLayers     = {}) -> Result;

    /**
     * @brief Setup required extensions and layers based on feature requirements and enumerate supported ones
     * 
     * @param createInfo Creation info with required features
     * @param enabledExtensions Output vector to populate with required extension names
     * @param enabledLayers Output vector to populate with required layer names
     * @param supportedExtensions Output set to populate with available extensions
     * @param supportedLayers Output set to populate with available layers
     * @return Result indicating success or failure with error details
     */
    static auto setupFeatures(InstanceCreateInfo& createInfo, SmallVector<const char*>& enabledExtensions,
                              SmallVector<const char*>& enabledLayers, HashSet<std::string>& supportedExtensions,
                              HashSet<std::string>& supportedLayers) -> Result;

#ifdef APH_DEBUG
    ::vk::DebugUtilsMessengerEXT m_debugMessenger{};
#endif
    SmallVector<PhysicalDevice*> m_physicalDevices{};
    ThreadSafeObjectPool<PhysicalDevice> m_physicalDevicePools;
};
} // namespace aph::vk
