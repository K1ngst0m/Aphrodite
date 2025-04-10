#include "physicalDevice.h"
#include "vkUtils.h"

namespace aph::vk
{

namespace
{
// Define the feature entries with all validation and setup logic in an anonymous namespace
auto getFeatureEntries()
{
    static auto featureEntries = std::to_array<PhysicalDevice::FeatureEntry>(
        { { .name = "Default Required",
            .isRequired = [](const GPUFeature& required) { return true; },
            .isSupported = [](const GPUFeature& supported) { return true; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                if (required)
                {
                    // Setup common features
                    auto& extDynamicState3 =
                        device->requestFeatures<::vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT>();
                    extDynamicState3.extendedDynamicState3ColorBlendEquation = VK_TRUE;

                    auto& shaderObjectFeatures = device->requestFeatures<::vk::PhysicalDeviceShaderObjectFeaturesEXT>();
                    shaderObjectFeatures.shaderObject = VK_TRUE;

                    auto& sync2Features = device->requestFeatures<::vk::PhysicalDeviceSynchronization2Features>();
                    sync2Features.synchronization2 = VK_TRUE;

                    auto& timelineSemaphoreFeatures =
                        device->requestFeatures<::vk::PhysicalDeviceTimelineSemaphoreFeatures>();
                    timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

                    auto& maintenance4Features = device->requestFeatures<::vk::PhysicalDeviceMaintenance4Features>();
                    maintenance4Features.maintenance4 = VK_TRUE;

                    // Request Inline Uniform Block Features
                    auto& inlineUniformBlockFeature =
                        device->requestFeatures<::vk::PhysicalDeviceInlineUniformBlockFeaturesEXT>();
                    inlineUniformBlockFeature.inlineUniformBlock = VK_TRUE;

                    // Request Dynamic Rendering Features
                    auto& dynamicRenderingFeature =
                        device->requestFeatures<::vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
                    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

                    // Request Host Query Reset Features
                    auto& hostQueryResetFeature = device->requestFeatures<::vk::PhysicalDeviceHostQueryResetFeatures>();
                    hostQueryResetFeature.hostQueryReset = VK_TRUE;

                    auto& deviceAddressFeatures =
                        device->requestFeatures<::vk::PhysicalDeviceBufferDeviceAddressFeatures>();
                    deviceAddressFeatures.setBufferDeviceAddress(::vk::True);
                }
            },
            .extensionNames = { VK_EXT_SHADER_OBJECT_EXTENSION_NAME, VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
                                VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
                                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
                                VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME },
            .isCritical = true },
          { .name = "Capture Disabled",
            .isRequired = [](const GPUFeature& required) { return true; },
            .isSupported = [](const GPUFeature& supported) { return !supported.capture; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                if (required)
                {
                    auto& descriptorBufferFeatures =
                        device->requestFeatures<::vk::PhysicalDeviceDescriptorBufferFeaturesEXT>();
                    descriptorBufferFeatures.descriptorBuffer = VK_TRUE;
                    descriptorBufferFeatures.descriptorBufferPushDescriptors = VK_TRUE;

                    auto& maintence5 = device->requestFeatures<::vk::PhysicalDeviceMaintenance5FeaturesKHR>();
                    maintence5.maintenance5 = VK_TRUE;
                }
            },
            .extensionNames = { VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME, VK_KHR_MAINTENANCE_5_EXTENSION_NAME },
            .isCritical = true },
          { .name = "Ray Tracing",
            .isRequired = [](const GPUFeature& required) { return required.rayTracing; },
            .isSupported = [](const GPUFeature& supported) { return supported.rayTracing; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                if (required)
                {
                    // Setup ray tracing features
                    auto& asFeature = device->requestFeatures<::vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
                    asFeature.accelerationStructure = VK_TRUE;

                    auto& rtPipelineFeature =
                        device->requestFeatures<::vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
                    rtPipelineFeature.rayTracingPipeline = VK_TRUE;

                    auto& rayQueryFeature = device->requestFeatures<::vk::PhysicalDeviceRayQueryFeaturesKHR>();
                    rayQueryFeature.rayQuery = VK_TRUE;
                }
            },
            .extensionNames = { VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME },
            .isCritical = true },
          { .name = "Mesh Shading",
            .isRequired = [](const GPUFeature& required) { return required.meshShading; },
            .isSupported = [](const GPUFeature& supported) { return supported.meshShading; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                if (required)
                {
                    // Setup mesh shader features
                    auto& meshShaderFeature = device->requestFeatures<::vk::PhysicalDeviceMeshShaderFeaturesEXT>();
                    meshShaderFeature.taskShader = VK_TRUE;
                    meshShaderFeature.meshShader = VK_TRUE;
                    meshShaderFeature.meshShaderQueries = VK_FALSE;
                    meshShaderFeature.multiviewMeshShader = VK_FALSE;
                    meshShaderFeature.primitiveFragmentShadingRateMeshShader = VK_FALSE;
                }
            },
            .extensionNames = { VK_EXT_MESH_SHADER_EXTENSION_NAME },
            .isCritical = true },
          { .name = "Multi Draw Indirect",
            .isRequired = [](const GPUFeature& required) { return required.multiDrawIndirect; },
            .isSupported = [](const GPUFeature& supported) { return supported.multiDrawIndirect; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                if (required)
                {
                    // Setup multi-draw indirect features
                    auto& multiDrawFeature = device->requestFeatures<::vk::PhysicalDeviceMultiDrawFeaturesEXT>();
                    multiDrawFeature.multiDraw = VK_TRUE;
                }
            },
            .extensionNames = { VK_EXT_MULTI_DRAW_EXTENSION_NAME, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME },
            .isCritical = true },
          { .name = "Tessellation Support",
            .isRequired = [](const GPUFeature& required) { return required.tessellationSupported; },
            .isSupported = [](const GPUFeature& supported) { return supported.tessellationSupported; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                // Tessellation is part of core Vulkan, just ensure the feature is enabled
                if (required)
                {
                    auto features = device->getHandle().getFeatures();
                    features.tessellationShader = VK_TRUE;
                }
            },
            .extensionNames = {}, // No extension needed, part of core
            .isCritical = true },
          { .name = "Sampler Anisotropy",
            .isRequired = [](const GPUFeature& required) { return required.samplerAnisotropy; },
            .isSupported = [](const GPUFeature& supported) { return supported.samplerAnisotropy; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                // Anisotropy is part of core Vulkan, just ensure the feature is enabled
                if (required)
                {
                    auto features = device->getHandle().getFeatures();
                    features.samplerAnisotropy = VK_TRUE;
                }
            },
            .extensionNames = {}, // No extension needed, part of core
            .isCritical = true },
          { .name = "Bindless",
            .isRequired = [](const GPUFeature& required) { return required.bindless; },
            .isSupported = [](const GPUFeature& supported) { return supported.bindless; },
            .enableFeature =
                [](PhysicalDevice* device, bool required)
            {
                if (required)
                {
                    // Setup bindless features
                    auto& descriptorIndexingFeatures =
                        device->requestFeatures<::vk::PhysicalDeviceDescriptorIndexingFeatures>();
                    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = ::vk::True;
                    descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = ::vk::True;
                    descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = ::vk::True;
                    descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = ::vk::True;
                    descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = ::vk::True;
                    descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = ::vk::True;
                    descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = ::vk::True;
                }
            },
            .extensionNames = { VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME },
            .isCritical = true },
          {
              .name = "Variable Rate Shading",
              .isRequired = [](const GPUFeature& required) { return required.variableRateShading; },
              .isSupported = [](const GPUFeature& supported) { return supported.variableRateShading; },
              .enableFeature =
                  [](PhysicalDevice* device, bool required)
              {
                  if (required)
                  {
                      // Setup VRS features
                      auto& vrsFeatures = device->requestFeatures<::vk::PhysicalDeviceFragmentShadingRateFeaturesKHR>();
                      vrsFeatures.pipelineFragmentShadingRate = VK_TRUE;
                      vrsFeatures.attachmentFragmentShadingRate = VK_TRUE;
                      vrsFeatures.primitiveFragmentShadingRate = VK_TRUE;
                  }
              },
              .extensionNames = { VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME },
              .isCritical = false // Not critical - application can run without it
          } });
    return featureEntries;
}
} // namespace

PhysicalDevice::PhysicalDevice(HandleType handle)
    : ResourceHandle(handle)
{
    const auto& features2 =
        m_handle.getFeatures2<::vk::PhysicalDeviceFeatures2, ::vk::PhysicalDeviceVulkan12Features>();
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
        auto* settings       = &m_properties;
        auto* gpuFeatures    = &features2.get<::vk::PhysicalDeviceFeatures2>();
        auto* gpuFeatures12  = &features2.get<::vk::PhysicalDeviceVulkan12Features>();

        {
            const auto& limits                        = gpuProperties2->properties.limits;
            settings->uniformBufferAlignment          = (uint32_t)limits.minUniformBufferOffsetAlignment;
            settings->uploadBufferTextureAlignment    = (uint32_t)limits.optimalBufferCopyOffsetAlignment;
            settings->uploadBufferTextureRowAlignment = (uint32_t)limits.optimalBufferCopyRowPitchAlignment;
            settings->maxVertexInputBindings          = limits.maxVertexInputBindings;
            settings->maxBoundDescriptorSets          = limits.maxBoundDescriptorSets;
            settings->timestampPeriod                 = limits.timestampPeriod;

            settings->waveLaneCount       = subgroupProperties.subgroupSize;
            settings->waveOpsSupportFlags = WaveOpsSupport::None;
            auto supportedOp              = subgroupProperties.supportedOperations;
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
            settings->feature.multiDrawIndirect     = gpuFeatures->features.multiDrawIndirect;
            settings->feature.tessellationSupported = gpuFeatures->features.tessellationShader;
            settings->feature.samplerAnisotropy     = gpuFeatures->features.samplerAnisotropy;

            settings->feature.meshShading = false;
            settings->feature.rayTracing  = false;
            settings->feature.bindless    = false;

            settings->feature.meshShading = checkExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME);
            settings->feature.rayTracing  = checkExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            // TODO
            settings->feature.bindless = gpuFeatures12->descriptorIndexing;
        }

        {
            char buffer[1024];

            std::snprintf(buffer, sizeof(buffer), "0x%08x", gpuProperties2->properties.deviceID);
            settings->GpuVendorPreset.modelId = buffer;

            std::snprintf(buffer, sizeof(buffer), "0x%08x", gpuProperties2->properties.vendorID);
            settings->GpuVendorPreset.vendorId = buffer;

            settings->GpuVendorPreset.gpuName = std::string{gpuProperties2->properties.deviceName};

            // driver info
            std::snprintf(buffer, sizeof(buffer), "%s - %s", driverProperties.driverInfo.data(),
                          driverProperties.driverName.data());
            settings->GpuVendorPreset.gpuDriverVersion = buffer;
        }

        // TODO: Fix once vulkan adds support for revision ID
        settings->GpuVendorPreset.revisionId = "0x00";
    }
}

bool PhysicalDevice::validateFeatures(const GPUFeature& requiredFeatures)
{
    const auto& entries       = getFeatureEntries();
    bool allFeaturesSupported = true;

    for (const auto& entry : entries)
    {
        // Skip entries that aren't required by the application
        if (!entry.isRequired(requiredFeatures))
            continue;

        // Check if the required feature is supported by the hardware
        if (!entry.isSupported(m_properties.feature))
        {
            // Report critical feature failure
            if (entry.isCritical)
            {
                VK_LOG_ERR("Critical GPU feature '%s' not supported by hardware", entry.name.data());
                allFeaturesSupported = false;
            }
            else
            {
                VK_LOG_WARN("Optional GPU feature '%s' not supported by hardware", entry.name.data());
            }
        }
    }

    return allFeaturesSupported;
}

void PhysicalDevice::setupRequiredExtensions(const GPUFeature& requiredFeatures,
                                             SmallVector<const char*>& requiredExtensions)
{
    const auto& entries = getFeatureEntries();

    for (const auto& entry : entries)
    {
        if (entry.isRequired(requiredFeatures))
        {
            for (const auto& extension : entry.extensionNames)
            {
                requiredExtensions.push_back(extension.data());
            }
        }
    }
}

void PhysicalDevice::enableFeatures(const GPUFeature& requiredFeatures)
{
    const auto featureEntries = getFeatureEntries();

    // Enable feature-specific functionality
    for (const auto& entry : featureEntries)
    {
        if (entry.isRequired(requiredFeatures))
        {
            entry.enableFeature(this, true);
        }
    }
}

Format PhysicalDevice::findSupportedFormat(ArrayProxy<Format> candidates, ::vk::ImageTiling tiling,
                                           ::vk::FormatFeatureFlags features) const
{
    for (Format format : candidates)
    {
        // TODO cast function
        auto vkFormat = static_cast<::vk::Format>(utils::VkCast(format));
        auto props    = m_handle.getFormatProperties(vkFormat);

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

std::size_t PhysicalDevice::getUniformBufferPaddingSize(size_t originalSize) const
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = m_handle.getProperties().limits.minUniformBufferOffsetAlignment;
    size_t alignedSize     = originalSize;
    return aph::utils::paddingSize(alignedSize, minUboAlignment);
}

} // namespace aph::vk
