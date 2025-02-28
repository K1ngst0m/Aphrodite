#include "physicalDevice.h"
#include "vkUtils.h"

namespace aph::vk
{
PhysicalDevice::PhysicalDevice(HandleType handle)
    : ResourceHandle(handle)
{
    const auto& features2 = m_handle.getFeatures2();
    const auto& properties2 =
        m_handle.getProperties2<::vk::PhysicalDeviceProperties2, ::vk::PhysicalDeviceDriverPropertiesKHR,
                                ::vk::PhysicalDeviceSubgroupProperties>();

    auto driverProperties = properties2.get<::vk::PhysicalDeviceDriverPropertiesKHR>();
    ::vk::PhysicalDeviceSubgroupProperties subgroupProperties =
        properties2.get<::vk::PhysicalDeviceSubgroupProperties>();

    {
        auto [result, exts] = m_handle.enumerateDeviceExtensionProperties();
        VK_VR(result);
        for (const auto& ext : exts)
        {
            m_supportedExtensions.insert(ext.extensionName);
        }
    }

    {
        auto* gpuProperties2 = &properties2.get<::vk::PhysicalDeviceProperties2>();
        auto* settings = &m_properties;
        auto* gpuFeatures = &features2;

        {
            const auto& limits = gpuProperties2->properties.limits;
            settings->uniformBufferAlignment = (uint32_t)limits.minUniformBufferOffsetAlignment;
            settings->uploadBufferTextureAlignment = (uint32_t)limits.optimalBufferCopyOffsetAlignment;
            settings->uploadBufferTextureRowAlignment = (uint32_t)limits.optimalBufferCopyRowPitchAlignment;
            settings->maxVertexInputBindings = limits.maxVertexInputBindings;
            settings->maxBoundDescriptorSets = limits.maxBoundDescriptorSets;
            settings->timestampPeriod = limits.timestampPeriod;

            settings->waveLaneCount = subgroupProperties.subgroupSize;
            settings->waveOpsSupportFlags = WaveOpsSupport::None;
            auto supportedOp = subgroupProperties.supportedOperations;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eBasic)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Basic;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eVote)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Vote;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eArithmetic)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Arithmetic;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eBallot)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Ballot;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eShuffle)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Shuffle;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eShuffleRelative)
                settings->waveOpsSupportFlags |= WaveOpsSupport::ShuffleRelative;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eClustered)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Clustered;
            if (supportedOp & ::vk::SubgroupFeatureFlagBits::eQuad)
                settings->waveOpsSupportFlags |= WaveOpsSupport::Quad;
        }

        // feature support
        {
            settings->feature.multiDrawIndirect = gpuFeatures->features.multiDrawIndirect;
            settings->feature.tessellationSupported = gpuFeatures->features.tessellationShader;
            settings->feature.samplerAnisotropySupported = gpuFeatures->features.samplerAnisotropy;

            settings->feature.meshShading = false;
            settings->feature.rayTracing = false;

            settings->feature.meshShading = checkExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME);
            settings->feature.rayTracing = checkExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                                   VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                                   VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        }

        {
            char buffer[1024];

            std::snprintf(buffer, sizeof(buffer), "0x%08x", gpuProperties2->properties.deviceID);
            settings->GpuVendorPreset.modelId = buffer;

            std::snprintf(buffer, sizeof(buffer), "0x%08x", gpuProperties2->properties.vendorID);
            settings->GpuVendorPreset.vendorId = buffer;

            settings->GpuVendorPreset.gpuName = std::string{ gpuProperties2->properties.deviceName };

            // driver info
            std::snprintf(buffer, sizeof(buffer), "%s - %s", driverProperties.driverInfo.data(),
                          driverProperties.driverName.data());
            settings->GpuVendorPreset.gpuDriverVersion = buffer;
        }

        // TODO: Fix once vulkan adds support for revision ID
        settings->GpuVendorPreset.revisionId = "0x00";
    }
}

uint32_t PhysicalDevice::findMemoryType(ImageDomain domain, uint32_t mask) const
{
    uint32_t desired = 0, fallback = 0;
    switch (domain)
    {
    case ImageDomain::Device:
        desired = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        fallback = 0;
        break;

    case ImageDomain::Transient:
        desired = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        fallback = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case ImageDomain::LinearHostCached:
        desired = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        fallback = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;

    case ImageDomain::LinearHost:
        desired = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        fallback = 0;
        break;
    }

    uint32_t index = findMemoryType(static_cast<::vk::MemoryPropertyFlags>(desired), mask);
    if (index != UINT32_MAX)
        return index;

    index = findMemoryType(static_cast<::vk::MemoryPropertyFlags>(fallback), mask);
    if (index != UINT32_MAX)
        return index;

    return UINT32_MAX;
}

uint32_t PhysicalDevice::findMemoryType(BufferDomain domain, uint32_t mask) const
{
    uint32_t prio[3] = {};

    switch (domain)
    {
    case BufferDomain::Device:
        prio[0] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case BufferDomain::LinkedDeviceHost:
        prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        prio[2] = prio[1];
        break;

    case BufferDomain::Host:
        prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        prio[2] = prio[1];
        break;

    case BufferDomain::CachedHost:
        prio[0] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        prio[1] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        prio[2] = prio[1];
        break;
    }

    for (auto& p : prio)
    {
        uint32_t index = findMemoryType(static_cast<::vk::MemoryPropertyFlags>(p), mask);
        if (index != UINT32_MAX)
            return index;
    }

    return UINT32_MAX;
}

uint32_t PhysicalDevice::findMemoryType(::vk::MemoryPropertyFlags required, uint32_t mask) const
{
    const auto& memoryProperties = m_handle.getMemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((1u << i) & mask)
        {
            auto flags = memoryProperties.memoryTypes[i].propertyFlags;
            if ((flags & required) == required)
                return i;
        }
    }

    return UINT32_MAX;
}

Format PhysicalDevice::findSupportedFormat(const std::vector<Format>& candidates, ::vk::ImageTiling tiling,
                                           ::vk::FormatFeatureFlags features) const
{
    for (Format format : candidates)
    {
        // TODO cast function
        auto vkFormat = static_cast<::vk::Format>(utils::VkCast(format));
        auto props = m_handle.getFormatProperties(vkFormat);

        if (tiling == ::vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }

        if (tiling == ::vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    assert("failed to find supported format!");
    return Format::Undefined;
}

size_t PhysicalDevice::padUniformBufferSize(size_t originalSize) const
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = m_handle.getProperties().limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0)
    {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

} // namespace aph::vk
