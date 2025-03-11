#pragma once

#include "common/hash.h"
#include "instance.h"

namespace aph::vk
{

class PhysicalDevice : public ResourceHandle<::vk::PhysicalDevice>
{
    friend class Device;

public:
    PhysicalDevice(HandleType handle);
    Format findSupportedFormat(const std::vector<Format>& candidates, ::vk::ImageTiling tiling,
                               ::vk::FormatFeatureFlags features) const;
    std::size_t getUniformBufferPaddingSize(size_t originalSize) const;
    const GPUProperties& getProperties() const
    {
        return m_properties;
    }

    template <typename... Extensions>
        requires(std::convertible_to<Extensions, std::string_view> && ...)
    bool checkExtensionSupported(Extensions&&... exts) const
    {
        auto isSupported = [this](std::string_view ext) -> bool
        { return m_supportedExtensions.contains(std::string{ ext }); };
        return (isSupported(std::forward<Extensions>(exts)) && ...);
    }

    template <typename T>
    T& requestFeatures()
    {
        auto features = m_handle.getFeatures2<::vk::PhysicalDeviceFeatures2, T>();
        auto requiredFeature = features.template get<T>();

        const ::vk::StructureType type = requiredFeature.sType;
        if (m_requestedFeatures.count(type))
        {
            return *std::static_pointer_cast<T>(m_requestedFeatures.at(type));
        }

        auto extensionPtr = std::make_shared<T>(requiredFeature);
        m_requestedFeatures.insert({ type, extensionPtr });
        if (m_pLastRequestedFeature)
        {
            extensionPtr->pNext = m_pLastRequestedFeature.get();
        }
        m_pLastRequestedFeature = extensionPtr;
        return *extensionPtr;
    }

    void* getRequestedFeatures() const
    {
        return m_pLastRequestedFeature.get();
    }

private:
    GPUProperties m_properties = {};
    HashSet<std::string> m_supportedExtensions = {};
    std::shared_ptr<void> m_pLastRequestedFeature = {};
    HashMap<::vk::StructureType, std::shared_ptr<void>> m_requestedFeatures;
};

} // namespace aph::vk
