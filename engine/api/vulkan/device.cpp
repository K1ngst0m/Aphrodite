#include "device.h"
#include "common/profiler.h"
#include "deviceAllocator.h"

const VkAllocationCallbacks* gVkAllocator = aph::vk::vkAllocator();

namespace aph::vk
{
Device::Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle) :
    ResourceHandle(handle, createInfo),
    m_physicalDevice(pPhysicalDevice),
    m_resourcePool(this)
{
}

std::unique_ptr<Device> Device::Create(const DeviceCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();
    PhysicalDevice* physicalDevice = createInfo.pPhysicalDevice;

    const auto& queueFamilyProperties = physicalDevice->m_queueFamilyProperties;
    const auto  queueFamilyCount      = queueFamilyProperties.size();

    // Allocate handles for all available queues.
    SmallVector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilyCount);
    SmallVector<SmallVector<float>>      priorities(queueFamilyCount);
    for(auto i = 0U; i < queueFamilyCount; ++i)
    {
        const float defaultPriority = 1.0f;
        priorities[i].resize(queueFamilyProperties[i].queueCount, defaultPriority);

        queueCreateInfos[i] = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = i,
            .queueCount       = queueFamilyProperties[i].queueCount,
            .pQueuePriorities = priorities[i].data(),
        };
    }

    // verify feature support
    {
        const auto& requiredFeature = createInfo.enabledFeatures;
        const auto& supportFeature  = physicalDevice->getSettings().feature;

        if(requiredFeature.meshShading && !supportFeature.meshShading)
        {
            CM_LOG_ERR("Mesh Shading feature not supported!");
            APH_ASSERT(false);
        }
        if(requiredFeature.multiDrawIndirect && !supportFeature.multiDrawIndirect)
        {
            CM_LOG_ERR("Multi Draw Indrect not supported!");
            APH_ASSERT(false);
        }
        if(requiredFeature.tessellationSupported && !supportFeature.tessellationSupported)
        {
            CM_LOG_ERR("some gpu feature not supported!");
            APH_ASSERT(false);
        }
        if(requiredFeature.samplerAnisotropySupported && !supportFeature.samplerAnisotropySupported)
        {
            CM_LOG_ERR("some gpu feature not supported!");
            APH_ASSERT(false);
        }
    }

    auto& sync2Features = physicalDevice->requestFeatures<VkPhysicalDeviceSynchronization2Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES);
    sync2Features.synchronization2 = VK_TRUE;

    auto& timelineSemaphoreFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceTimelineSemaphoreFeatures>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES);
    timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

    auto& descriptorBufferFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceDescriptorBufferFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT);
    descriptorBufferFeatures.descriptorBuffer = VK_TRUE;
    descriptorBufferFeatures.descriptorBufferPushDescriptors = VK_TRUE;

    auto& maintenance4Features = physicalDevice->requestFeatures<VkPhysicalDeviceMaintenance4Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES);
    maintenance4Features.maintenance4 = VK_TRUE;

    auto& descriptorIndexingFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceDescriptorIndexingFeatures>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES);
    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

    // Request Inline Uniform Block Features EXT
    auto& inlineUniformBlockFeature = physicalDevice->requestFeatures<VkPhysicalDeviceInlineUniformBlockFeaturesEXT>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT);
    inlineUniformBlockFeature.inlineUniformBlock = VK_TRUE;

    // Request Dynamic Rendering Features KHR
    auto& dynamicRenderingFeature = physicalDevice->requestFeatures<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    // Request Host Query Reset Features
    auto& hostQueryResetFeature = physicalDevice->requestFeatures<VkPhysicalDeviceHostQueryResetFeatures>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES);
    hostQueryResetFeature.hostQueryReset = VK_TRUE;


    std::vector<const char*> exts;
    {
        const auto& feature = createInfo.enabledFeatures;
        if(feature.meshShading)
        {
            // Request Mesh Shader Features EXT
            auto& meshShaderFeature = physicalDevice->requestFeatures<VkPhysicalDeviceMeshShaderFeaturesEXT>(
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT);

            meshShaderFeature.taskShader = VK_TRUE;
            meshShaderFeature.meshShader = VK_TRUE;
            exts.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }

        if(feature.multiDrawIndirect)
        {
            // Request Multi-Draw Features EXT
            auto& multiDrawFeature = physicalDevice->requestFeatures<VkPhysicalDeviceMultiDrawFeaturesEXT>(
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT);
            multiDrawFeature.multiDraw = VK_TRUE;
            exts.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
            exts.push_back(VK_EXT_MULTI_DRAW_EXTENSION_NAME);
        }

        exts.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
        exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        exts.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        exts.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        exts.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
        exts.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
    }

    // Enable all physical device available features.
    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(physicalDevice->getHandle(), &supportedFeatures);
    VkPhysicalDeviceFeatures2 supportedFeatures2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    vkGetPhysicalDeviceFeatures2(physicalDevice->getHandle(), &supportedFeatures2);

    supportedFeatures.sampleRateShading = VK_TRUE;
    supportedFeatures.samplerAnisotropy = VK_TRUE;

    supportedFeatures2.pNext    = physicalDevice->getRequestedFeatures();
    supportedFeatures2.features = supportedFeatures;

    // Create the Vulkan device.
    VkDeviceCreateInfo deviceCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &supportedFeatures2,
        .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(exts.size()),
        .ppEnabledExtensionNames = exts.data(),
    };

    VkDevice handle = VK_NULL_HANDLE;
    _VR(vkCreateDevice(physicalDevice->getHandle(), &deviceCreateInfo, gVkAllocator, &handle));
    _VR(utils::setDebugObjectName(handle, VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(handle), "Device"))

    // Initialize Device class.
    auto device = std::unique_ptr<Device>(new Device(createInfo, physicalDevice, handle));
    volkLoadDeviceTable(&device->m_table, handle);
    device->m_supportedFeatures = supportedFeatures;
    // TODO
    device->m_resourcePool.gpu = new VMADeviceAllocator(createInfo.pInstance, device.get());

    {
        for(uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
        {
            auto& queueFamily = physicalDevice->m_queueFamilyProperties[queueFamilyIndex];
            auto  queueFlags  = queueFamily.queueFlags;

            QueueType queueType = QueueType::Unsupport;
            // universal queue
            if(queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                VK_LOG_DEBUG("create graphics queue %lu", queueFamilyIndex);
                queueType = QueueType::Graphics;
            }
            // compute queue
            else if(queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                VK_LOG_DEBUG("Found compute queue %lu", queueFamilyIndex);
                queueType = QueueType::Compute;
            }
            // transfer queue
            else if(queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                VK_LOG_DEBUG("Found transfer queue %lu", queueFamilyIndex);
                queueType = QueueType::Transfer;
            }

            for(auto queueIndex = 0U; queueIndex < queueCreateInfos[queueFamilyIndex].queueCount; ++queueIndex)
            {
                VkQueue queue = VK_NULL_HANDLE;
                device->m_table.vkGetDeviceQueue(handle, queueFamilyIndex, queueIndex, &queue);
                device->m_queues[queueType].push_back(device->m_resourcePool.queue.allocate(device.get(),
                                                                                                queue,
                                                                                                queueFamilyIndex,
                                                                                                queueIndex,
                                                                                                queueFamilyProperties[queueFamilyIndex]));
            }
        }
    }

    // Return success.
    return device;
}

void Device::Destroy(Device* pDevice)
{
    APH_PROFILER_SCOPE();
    // TODO
    delete pDevice->m_resourcePool.gpu;

    pDevice->m_resourcePool.pipeline.clear();
    pDevice->m_resourcePool.program.clear();
    pDevice->m_resourcePool.syncPrimitive.clear();
    pDevice->m_resourcePool.commandPool.clear();

    if(pDevice->m_handle)
    {
        pDevice->m_table.vkDestroyDevice(pDevice->m_handle, gVkAllocator);
    }
}

VkFormat Device::getDepthFormat() const
{
    APH_PROFILER_SCOPE();
    return m_physicalDevice->findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

Result Device::create(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    switch(createInfo.type)
    {
    case PipelineType::Geometry:
    {
        auto vs = createInfo.geometry.pVertex;
        auto fs = createInfo.geometry.pFragment;
        APH_ASSERT(vs);
        APH_ASSERT(fs);

        *ppProgram = m_resourcePool.program.allocate(this, vs, fs, createInfo.samplerBank);
    }
    break;
    case PipelineType::Mesh:
    {
        auto ms = createInfo.mesh.pMesh;
        auto ts = createInfo.mesh.pTask;
        auto fs = createInfo.mesh.pFragment;
        APH_ASSERT(ms);
        APH_ASSERT(fs);

        *ppProgram = m_resourcePool.program.allocate(this, ms, ts, fs, createInfo.samplerBank);
    }
    break;
    case PipelineType::Compute:
    {
        auto cs = createInfo.compute.pCompute;
        APH_ASSERT(cs);
        *ppProgram = m_resourcePool.program.allocate(this, cs, createInfo.samplerBank);
    }
    break;
    case PipelineType::RayTracing:
    {
        APH_ASSERT(false);
        return Result::RuntimeError;
    }
    break;
    default:
    {
        APH_ASSERT(false);
        return Result::RuntimeError;
    }
    }
    return Result::Success;
}

Result Device::create(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    VkImageViewCreateInfo info{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .image    = createInfo.pImage->getHandle(),
        .viewType = createInfo.viewType,
        .format   = utils::VkCast(createInfo.format),
    };
    info.subresourceRange = {
        .aspectMask     = utils::getImageAspect(utils::VkCast(createInfo.format)),
        .baseMipLevel   = createInfo.subresourceRange.baseMipLevel,
        .levelCount     = createInfo.subresourceRange.levelCount,
        .baseArrayLayer = createInfo.subresourceRange.baseArrayLayer,
        .layerCount     = createInfo.subresourceRange.layerCount,
    };
    memcpy(&info.components, &createInfo.components, sizeof(VkComponentMapping));

    VkImageView handle = VK_NULL_HANDLE;
    _VR(m_table.vkCreateImageView(getHandle(), &info, gVkAllocator, &handle));
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(handle),
                                  debugName))

    *ppImageView = m_resourcePool.imageView.allocate(createInfo, handle);

    return Result::Success;
}

Result Device::create(const BufferCreateInfo& createInfo, Buffer** ppBuffer, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = createInfo.size,
        .usage       = createInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    m_table.vkCreateBuffer(getHandle(), &bufferInfo, vkAllocator(), &buffer);
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer), debugName))
    *ppBuffer = m_resourcePool.buffer.allocate(createInfo, buffer);
    m_resourcePool.gpu->allocate(*ppBuffer);

    return Result::Success;
}

Result Device::create(const ImageCreateInfo& createInfo, Image** ppImage, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    VkImageCreateInfo imageCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = createInfo.flags,
        .imageType     = createInfo.imageType,
        .format        = utils::VkCast(createInfo.format),
        .mipLevels     = createInfo.mipLevels,
        .arrayLayers   = createInfo.arraySize,
        .samples       = static_cast<VkSampleCountFlagBits>(createInfo.samples),
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = createInfo.usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    imageCreateInfo.extent.width  = createInfo.extent.width;
    imageCreateInfo.extent.height = createInfo.extent.height;
    imageCreateInfo.extent.depth  = createInfo.extent.depth;

    VkImage image;
    m_table.vkCreateImage(getHandle(), &imageCreateInfo, vkAllocator(), &image);
    *ppImage = m_resourcePool.image.allocate(this, createInfo, image);
    m_resourcePool.gpu->allocate(*ppImage);

    return Result::Success;
}

void Device::destroy(ShaderProgram* pProgram)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.program.free(pProgram);
}

void Device::destroy(Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.gpu->free(pBuffer);
    m_table.vkDestroyBuffer(getHandle(), pBuffer->getHandle(), vkAllocator());
    m_resourcePool.buffer.free(pBuffer);
}

void Device::destroy(Image* pImage)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.gpu->free(pImage);
    m_table.vkDestroyImage(getHandle(), pImage->getHandle(), vkAllocator());
    m_resourcePool.image.free(pImage);
}

void Device::destroy(ImageView* pImageView)
{
    APH_PROFILER_SCOPE();
    m_table.vkDestroyImageView(m_handle, pImageView->getHandle(), gVkAllocator);
    m_resourcePool.imageView.free(pImageView);
}

Result Device::create(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    *ppSwapchain = new SwapChain(createInfo, this);
    return Result::Success;
}

void Device::destroy(SwapChain* pSwapchain)
{
    APH_PROFILER_SCOPE();
    m_table.vkDestroySwapchainKHR(getHandle(), pSwapchain->getHandle(), gVkAllocator);
    delete pSwapchain;
    pSwapchain = nullptr;
}

Queue* Device::getQueue(QueueType type, uint32_t queueIndex)
{
    APH_PROFILER_SCOPE();

    if(m_queues.count(type) && queueIndex < m_queues[type].size() && m_queues[type][queueIndex] != nullptr)
    {
        return m_queues[type][queueIndex];
    }

    const QueueType fallbackOrder[] = {QueueType::Transfer, QueueType::Compute, QueueType::Graphics};

    for(QueueType fallbackType : fallbackOrder)
    {
        if(queueIndex < m_queues[fallbackType].size() && m_queues[fallbackType][queueIndex] != nullptr)
        {
            // CM_LOG_WARN("Requested queue type [%s] (index %u) not available. Falling back to queue type %d.",
            //             aph::vk::utils::toString(type), queueIndex, aph::vk::utils::toString(fallbackType));
            return m_queues[fallbackType][queueIndex];
        }
    }

    if(type != QueueType::Graphics && type != QueueType::Compute && type != QueueType::Transfer)
    {
        CM_LOG_WARN("Unsupported queue type %d requested for index %u.", aph::vk::utils::toString(type), queueIndex);
    }
    else
    {
        CM_LOG_WARN("No available queue for requested type %d (index %u) nor in fallbacks.",
                    aph::vk::utils::toString(type), queueIndex);
    }

    return nullptr;
}

void Device::waitIdle()
{
    APH_PROFILER_SCOPE();
    m_table.vkDeviceWaitIdle(getHandle());
}

Pipeline* Device::acquirePipeline(const GraphicsPipelineCreateInfo& createInfo, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    Pipeline* pPipeline = m_resourcePool.pipeline.getPipeline(createInfo);
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_PIPELINE,
                                  reinterpret_cast<uint64_t>(pPipeline->getHandle()), debugName));
    return pPipeline;
}

Pipeline* Device::acquirePipeline(const ComputePipelineCreateInfo& createInfo, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    Pipeline* pPipeline = m_resourcePool.pipeline.getPipeline(createInfo);
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_PIPELINE,
                                  reinterpret_cast<uint64_t>(pPipeline->getHandle()), debugName));
    return pPipeline;
}

Result Device::waitForFence(const std::vector<Fence*>& fences, bool waitAll, uint32_t timeout)
{
    APH_PROFILER_SCOPE();
    SmallVector<VkFence> vkFences(fences.size());
    for(auto idx = 0; idx < fences.size(); ++idx)
    {
        vkFences[idx] = fences[idx]->getHandle();
    }
    return utils::getResult(m_table.vkWaitForFences(getHandle(), vkFences.size(), vkFences.data(),
                                                    waitAll ? VK_TRUE : VK_FALSE, UINT64_MAX));
}

Result Device::flushMemory(VkDeviceMemory memory, MemoryRange range)
{
    APH_PROFILER_SCOPE();
    if(range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }
    VkMappedMemoryRange mappedRange = {
        .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = memory,
        .offset = range.offset,
        .size   = range.size,
    };
    return utils::getResult(m_table.vkFlushMappedMemoryRanges(getHandle(), 1, &mappedRange));
}
Result Device::invalidateMemory(VkDeviceMemory memory, MemoryRange range)
{
    APH_PROFILER_SCOPE();
    if(range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }
    VkMappedMemoryRange mappedRange = {
        .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = memory,
        .offset = range.offset,
        .size   = range.size,
    };
    return utils::getResult(m_table.vkInvalidateMappedMemoryRanges(getHandle(), 1, &mappedRange));
}

Result Device::mapMemory(Buffer* pBuffer, void** ppMapped) const
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(ppMapped);
    return m_resourcePool.gpu->map(pBuffer, ppMapped);
}

void Device::unMapMemory(Buffer* pBuffer) const
{
    APH_PROFILER_SCOPE();
    m_resourcePool.gpu->unMap(pBuffer);
}

Result Device::create(const SamplerCreateInfo& createInfo, Sampler** ppSampler, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    VkSampler sampler = {};
    YcbcrData ycbcr;

    // default sampler lod values
    // used if not overriden by mSetLodRange or not Linear mipmaps
    float minSamplerLod = 0;
    float maxSamplerLod = createInfo.mipMapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR ? VK_LOD_CLAMP_NONE : 0;
    // user provided lods
    if(createInfo.setLodRange)
    {
        minSamplerLod = createInfo.minLod;
        maxSamplerLod = createInfo.maxLod;
    }

    VkSamplerCreateInfo ci{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = 0,
        .magFilter    = createInfo.magFilter,
        .minFilter    = createInfo.minFilter,
        .mipmapMode   = createInfo.mipMapMode,
        .addressModeU = createInfo.addressU,
        .addressModeV = createInfo.addressV,
        .addressModeW = createInfo.addressW,
        .mipLodBias   = createInfo.mipLodBias,
        .anisotropyEnable =
            (createInfo.maxAnisotropy > 0.0f && m_supportedFeatures.samplerAnisotropy) ? VK_TRUE : VK_FALSE,
        .maxAnisotropy           = createInfo.maxAnisotropy,
        .compareEnable           = createInfo.compareFunc != VK_COMPARE_OP_NEVER ? VK_TRUE : VK_FALSE,
        .compareOp               = createInfo.compareFunc,
        .minLod                  = minSamplerLod,
        .maxLod                  = maxSamplerLod,
        .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if(createInfo.pConvertInfo)
    {
        auto convertInfo = *createInfo.pConvertInfo;

        // Check format props
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(getPhysicalDevice()->getHandle(), utils::VkCast(convertInfo.format),
                                                &formatProperties);
            if(convertInfo.chromaOffsetX == VK_CHROMA_LOCATION_MIDPOINT)
            {
                APH_ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT);
            }
            else if(convertInfo.chromaOffsetX == VK_CHROMA_LOCATION_COSITED_EVEN)
            {
                APH_ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT);
            }
        }

        VkSamplerYcbcrConversionCreateInfo vkConvertInfo{
            .sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
            .pNext      = nullptr,
            .format     = utils::VkCast(convertInfo.format),
            .ycbcrModel = convertInfo.model,
            .ycbcrRange = convertInfo.range,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY},
            .xChromaOffset               = convertInfo.chromaOffsetX,
            .yChromaOffset               = convertInfo.chromaOffsetY,
            .chromaFilter                = convertInfo.chromaFilter,
            .forceExplicitReconstruction = convertInfo.forceExplicitReconstruction ? VK_TRUE : VK_FALSE,
        };

        _VR(m_table.vkCreateSamplerYcbcrConversion(getHandle(), &vkConvertInfo, gVkAllocator, &ycbcr.conversion));
        _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION,
                                      reinterpret_cast<uint64_t>(ycbcr.conversion), debugName))

        ycbcr.info.sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
        ycbcr.info.pNext      = nullptr;
        ycbcr.info.conversion = ycbcr.conversion;

        ci.pNext = &ycbcr.info;
    }

    _VR(m_table.vkCreateSampler(getHandle(), &ci, gVkAllocator, &sampler));
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(sampler), debugName))
    *ppSampler = m_resourcePool.sampler.allocate(this, createInfo, sampler);
    return Result::Success;
}

void Device::destroy(Sampler* pSampler)
{
    APH_PROFILER_SCOPE();
    m_table.vkDestroySampler(getHandle(), pSampler->getHandle(), gVkAllocator);
    m_resourcePool.sampler.free(pSampler);
}

double Device::getTimeQueryResults(VkQueryPool pool, uint32_t firstQuery, uint32_t secondQuery, TimeUnit unitType)
{
    APH_PROFILER_SCOPE();
    uint64_t firstTimeStamp, secondTimeStamp;

    m_table.vkGetQueryPoolResults(getHandle(), pool, firstQuery, 1, sizeof(uint64_t), &firstTimeStamp, sizeof(uint64_t),
                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    m_table.vkGetQueryPoolResults(getHandle(), pool, secondQuery, 1, sizeof(uint64_t), &secondTimeStamp,
                                  sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    uint64_t timeDifference = secondTimeStamp - firstTimeStamp;
    auto     period         = getPhysicalDevice()->getProperties().limits.timestampPeriod;
    auto     timeInSeconds  = timeDifference * period;

    switch(unitType)
    {
    case TimeUnit::Seconds:
        return timeInSeconds * 1e-9;
    case TimeUnit::MillSeconds:
        return timeInSeconds * 1e-6;
    case TimeUnit::MicroSeconds:
        return timeInSeconds * 1e-3;
    case TimeUnit::NanoSeconds:
        return timeInSeconds;
    default:
        APH_ASSERT(false);
        return timeInSeconds * 1e-9;
    }
}
Semaphore* Device::acquireSemaphore()
{
    APH_PROFILER_SCOPE();
    Semaphore* semaphore;
    m_resourcePool.syncPrimitive.acquireSemaphore(1, &semaphore);
    return semaphore;
}
Result Device::releaseSemaphore(Semaphore* semaphore)
{
    APH_PROFILER_SCOPE();
    if(semaphore != VK_NULL_HANDLE)
    {
        auto result = m_resourcePool.syncPrimitive.ReleaseSemaphores(1, &semaphore);
        if(result != VK_SUCCESS)
        {
            return Result::RuntimeError;
        }
    }
    return Result::Success;
}
Fence* Device::acquireFence(bool isSignaled)
{
    APH_PROFILER_SCOPE();
    Fence* pFence = {};
    m_resourcePool.syncPrimitive.acquireFence(&pFence, isSignaled);
    return pFence;
}
Result Device::releaseFence(Fence* pFence)
{
    APH_PROFILER_SCOPE();
    auto res = m_resourcePool.syncPrimitive.releaseFence(pFence);
    if(res != VK_SUCCESS)
    {
        return Result::RuntimeError;
    }
    return Result::Success;
}
CommandPool* Device::acquireCommandPool(const CommandPoolCreateInfo& info)
{
    APH_PROFILER_SCOPE();
    CommandPool* pool = {};
    APH_VR(m_resourcePool.commandPool.acquire(info, 1, &pool));
    return pool;
}
Result Device::releaseCommandPool(CommandPool* pPool)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.commandPool.release(1, &pPool);
    return Result::Success;
}
void Device::executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func,
                                   const std::vector<Semaphore*>& waitSems, const std::vector<Semaphore*>& signalSems,
                                   Fence* pFence)
{
    APH_PROFILER_SCOPE();
    CommandPool*   commandPool = acquireCommandPool({.queue = queue, .transient = true});
    CommandBuffer* cmd         = nullptr;
    APH_VR(commandPool->allocate(1, &cmd));

    _VR(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
    func(cmd);
    _VR(cmd->end());

    QueueSubmitInfo submitInfo{.commandBuffers = {cmd}, .waitSemaphores = waitSems, .signalSemaphores = signalSems};
    if(!pFence)
    {
        auto fence = acquireFence(false);
        APH_VR(queue->submit({submitInfo}, fence));
        fence->wait();
    }
    else
    {
        pFence = acquireFence(false);
        APH_VR(queue->submit({submitInfo}, pFence));
    }

    APH_VR(releaseCommandPool(commandPool));
}
}  // namespace aph::vk
