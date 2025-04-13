#pragma once

#include "api/gpuResource.h"
#include "common/hash.h"
#include "forward.h"
#include "instance.h"
#include "vkUtils.h"
#include <array>
#include <functional>
#include <span>
#include <string_view>

namespace aph::vk
{

class PhysicalDevice : public ResourceHandle<::vk::PhysicalDevice>
{
    friend class Device;

public:
    /**
     * @brief Structure representing a single GPU feature
     */
    struct FeatureEntry
    {
        // Feature identifier for debugging and error messages
        std::string_view name;

        // Function to check if the feature is required
        std::function<bool(const GPUFeature&)> isRequired;

        // Function to check if the feature is supported
        std::function<bool(const GPUFeature&)> isSupported;

        // Function to enable the feature in Vulkan's structure chain
        std::function<void(PhysicalDevice*, bool)> enableFeature;

        // Extensions related to this feature
        SmallVector<std::string_view> extensionNames;

        // Is this feature critical (will cause application to fail if not supported)
        bool isCritical = true;
    };

public:
    explicit PhysicalDevice(HandleType handle);
    auto findSupportedFormat(ArrayProxy<Format> candidates, ::vk::ImageTiling tiling,
                               ::vk::FormatFeatureFlags features) const -> Format;
    auto getUniformBufferPaddingSize(size_t originalSize) const -> std::size_t;
    auto getProperties() const -> const GPUProperties&;

    /**
     * @brief Validate all required features against supported features
     * 
     * @param requiredFeatures Features required by the application
     * @return true if all required features are supported
     */
    auto validateFeatures(const GPUFeature& requiredFeatures) -> bool;

    /**
     * @brief Setup required extensions based on feature requirements
     * 
     * @param requiredFeatures Features required by the application
     * @param requiredExtensions Vector to populate with required extension names
     */
    auto setupRequiredExtensions(const GPUFeature& requiredFeatures, SmallVector<const char*>& requiredExtensions) -> void;

    /**
     * @brief Enable features in the Vulkan structures before device creation
     * 
     * @param requiredFeatures Features required by the application
     */
    auto enableFeatures(const GPUFeature& requiredFeatures) -> void;

    template <typename... Extensions>
        requires(std::convertible_to<Extensions, std::string_view> && ...)
    auto checkExtensionSupported(Extensions&&... exts) const -> bool;

    template <typename T>
    auto requestFeatures() -> T&;

    auto getRequestedFeatures() const -> void*;

private:
    GPUProperties m_properties                    = {};
    HashSet<std::string> m_supportedExtensions    = {};
    std::shared_ptr<void> m_pLastRequestedFeature = {};
    HashMap<::vk::StructureType, std::shared_ptr<void>> m_requestedFeatures;
};

template <typename T>
inline auto PhysicalDevice::requestFeatures() -> T&
{
    auto features        = m_handle.getFeatures2<::vk::PhysicalDeviceFeatures2, T>();
    auto requiredFeature = features.template get<T>();

    const ::vk::StructureType type = requiredFeature.sType;
    if (m_requestedFeatures.count(type))
    {
        return *std::static_pointer_cast<T>(m_requestedFeatures.at(type));
    }

    auto extensionPtr = std::make_shared<T>(requiredFeature);
    m_requestedFeatures.insert({type, extensionPtr});
    if (m_pLastRequestedFeature)
    {
        extensionPtr->pNext = m_pLastRequestedFeature.get();
    }
    m_pLastRequestedFeature = extensionPtr;
    return *extensionPtr;
}

template <typename... Extensions>
    requires(std::convertible_to<Extensions, std::string_view> && ...)
inline auto PhysicalDevice::checkExtensionSupported(Extensions&&... exts) const -> bool
{
    auto isSupported = [this](std::string_view ext) -> bool
    { return m_supportedExtensions.contains(std::string{ext}); };
    return (isSupported(std::forward<Extensions>(exts)) && ...);
}

} // namespace aph::vk
