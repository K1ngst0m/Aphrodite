#include "physicalDevice.h"

namespace aph::vk
{

PhysicalDevice::PhysicalDevice(HandleType handle) : ResourceHandle(handle)
{
    {
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, nullptr);
        APH_ASSERT(queueFamilyCount > 0);
        m_queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(getHandle(), &queueFamilyCount, m_queueFamilyProperties.data());
        for(uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
        {
            auto& queueFamily = m_queueFamilyProperties[queueFamilyIndex];
            auto  queueFlags  = queueFamily.queueFlags;
            // universal queue
            if(queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                VK_LOG_DEBUG("Found graphics queue %lu", queueFamilyIndex);
                m_queueFamilyMap[QueueType::Graphics].push_back(queueFamilyIndex);
            }
            // compute queue
            else if(queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                VK_LOG_DEBUG("Found compute queue %lu", queueFamilyIndex);
                m_queueFamilyMap[QueueType::Compute].push_back(queueFamilyIndex);
            }
            // transfer queue
            else if(queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                VK_LOG_DEBUG("Found transfer queue %lu", queueFamilyIndex);
                m_queueFamilyMap[QueueType::Transfer].push_back(queueFamilyIndex);
            }
            // else
            // {
            //     APH_ASSERT(false);
            // }
        }
    }
    vkGetPhysicalDeviceProperties(getHandle(), &m_properties);
    vkGetPhysicalDeviceMemoryProperties(getHandle(), &m_memoryProperties);

    m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
#if VK_EXT_fragment_shader_interlock
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT};
    m_features2.pNext = &fragmentShaderInterlockFeatures;
#endif
    vkGetPhysicalDeviceFeatures2(getHandle(), &m_features2);

    m_driverProperties       = {};
    m_driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    // Get device properties
    VkPhysicalDeviceSubgroupProperties subgroupProperties = {};
    subgroupProperties.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    subgroupProperties.pNext                              = &m_driverProperties;

    m_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
    m_properties2.pNext = &subgroupProperties;
    vkGetPhysicalDeviceProperties2(getHandle(), &m_properties2);

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(getHandle(), nullptr, &extCount, nullptr);
    if(extCount > 0)
    {
        std::vector<VkExtensionProperties> extensions(extCount);
        if(vkEnumerateDeviceExtensionProperties(getHandle(), nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
        {
            for(auto ext : extensions)
            {
                m_supportedExtensions.emplace_back(ext.extensionName);
            }
        }
    }

    {
        auto* gpuProperties2 = &m_properties2;
        auto* gpuSettings    = &m_settings;
        auto* gpuFeatures    = &m_features2;

        {
            gpuSettings->uniformBufferAlignment =
                (uint32_t)gpuProperties2->properties.limits.minUniformBufferOffsetAlignment;
            gpuSettings->uploadBufferTextureAlignment =
                (uint32_t)gpuProperties2->properties.limits.optimalBufferCopyOffsetAlignment;
            gpuSettings->uploadBufferTextureRowAlignment =
                (uint32_t)gpuProperties2->properties.limits.optimalBufferCopyRowPitchAlignment;
            gpuSettings->maxVertexInputBindings = gpuProperties2->properties.limits.maxVertexInputBindings;

            gpuSettings->waveLaneCount       = subgroupProperties.subgroupSize;
            gpuSettings->waveOpsSupportFlags = WAVE_OPS_SUPPORT_FLAG_NONE;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_BASIC_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_VOTE_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_CLUSTERED_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_QUAD_BIT;
            if(subgroupProperties.supportedOperations & VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV)
                gpuSettings->waveOpsSupportFlags |= WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV;
        }

        // feature support
        {
            gpuSettings->feature.multiDrawIndirect          = gpuFeatures->features.multiDrawIndirect;
            gpuSettings->feature.tessellation      = gpuFeatures->features.tessellationShader;
            gpuSettings->feature.samplerAnisotropy = gpuFeatures->features.samplerAnisotropy;

            gpuSettings->feature.meshShading = false;
            for(const auto& ext : m_supportedExtensions)
            {
                if(ext == VK_EXT_MESH_SHADER_EXTENSION_NAME)
                {
                    gpuSettings->feature.meshShading = true;
                }
            }
        }

        {
            char buffer[1024];

            std::snprintf(buffer, sizeof(buffer), "0x%08x", gpuProperties2->properties.deviceID);
            gpuSettings->GpuVendorPreset.modelId = buffer;

            std::snprintf(buffer, sizeof(buffer), "0x%08x", gpuProperties2->properties.vendorID);
            gpuSettings->GpuVendorPreset.vendorId = buffer;

            gpuSettings->GpuVendorPreset.gpuName = gpuProperties2->properties.deviceName;

            // driver info
            std::snprintf(buffer, sizeof(buffer), "%s - %s", m_driverProperties.driverInfo,
                          m_driverProperties.driverName);
            gpuSettings->GpuVendorPreset.gpuDriverVersion = buffer;
        }

        // TODO: Fix once vulkan adds support for revision ID
        gpuSettings->GpuVendorPreset.revisionId = "0x00";
    }
}

bool PhysicalDevice::isExtensionSupported(std::string_view extension) const
{
    return (std::find(m_supportedExtensions.begin(), m_supportedExtensions.end(), extension) !=
            m_supportedExtensions.end());
}

uint32_t PhysicalDevice::findMemoryType(ImageDomain domain, uint32_t mask) const
{
    uint32_t desired = 0, fallback = 0;
    switch(domain)
    {
    case ImageDomain::Device:
        desired  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        fallback = 0;
        break;

    case ImageDomain::Transient:
        desired  = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        fallback = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case ImageDomain::LinearHostCached:
        desired  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        fallback = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;

    case ImageDomain::LinearHost:
        desired  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        fallback = 0;
        break;
    }

    uint32_t index = findMemoryType(desired, mask);
    if(index != UINT32_MAX)
        return index;

    index = findMemoryType(fallback, mask);
    if(index != UINT32_MAX)
        return index;

    return UINT32_MAX;
}

uint32_t PhysicalDevice::findMemoryType(BufferDomain domain, uint32_t mask) const
{
    uint32_t prio[3] = {};

    switch(domain)
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

    for(auto& p : prio)
    {
        uint32_t index = findMemoryType(p, mask);
        if(index != UINT32_MAX)
            return index;
    }

    return UINT32_MAX;
}

uint32_t PhysicalDevice::findMemoryType(VkMemoryPropertyFlags required, uint32_t mask) const
{
    for(uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
    {
        if((1u << i) & mask)
        {
            uint32_t flags = m_memoryProperties.memoryTypes[i].propertyFlags;
            if((flags & required) == required)
                return i;
        }
    }

    return UINT32_MAX;
}

VkFormat PhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                             VkFormatFeatureFlags features) const
{
    for(VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(getHandle(), format, &props);
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }

        if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    assert("failed to find supported format!");
    return {};
}

const std::vector<uint32_t>& PhysicalDevice::getQueueFamilyIndexByFlags(QueueType flags)
{
    for(auto i = static_cast<int>(flags); i >= 0; --i)
    {
        QueueType queueType = static_cast<QueueType>(i);

        if(m_queueFamilyMap.contains(queueType))
        {
            return m_queueFamilyMap.at(queueType);
        }
        CM_LOG_WARN("could not found queue type (transfer or compute), fallback to next.");
    }
    APH_ASSERT(false);
    return m_queueFamilyMap.at(QueueType::Graphics);
}

size_t PhysicalDevice::padUniformBufferSize(size_t originalSize) const
{
    // Calculate required alignment based on minimum device offset alignment
    size_t minUboAlignment = m_properties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize     = originalSize;
    if(minUboAlignment > 0)
    {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

VkPipelineStageFlags utils::determinePipelineStageFlags(PhysicalDevice* pGPU, VkAccessFlags accessFlags,
                                                        QueueType queueType)
{
    VkPipelineStageFlags flags = 0;

    auto* gpuSupport = &pGPU->getSettings();
    switch(queueType)
    {
    case aph::QueueType::Graphics:
    {
        if((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

        if((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
        {
            flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            if(gpuSupport->feature.tessellation)
            {
                flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
            }
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        }

        if((accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0)
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        if((accessFlags &
            (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        break;
    }
    case aph::QueueType::Compute:
    {
        if((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0 ||
           (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
           (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
           (accessFlags &
            (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        break;
    }
    case aph::QueueType::Transfer:
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    default:
        break;
    }

    // Compatible with both compute and graphics queues
    if((accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) != 0)
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    if((accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if((accessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT)) != 0)
        flags |= VK_PIPELINE_STAGE_HOST_BIT;

    if(flags == 0)
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flags;
}
}  // namespace aph::vk
