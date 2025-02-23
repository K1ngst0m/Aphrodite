#include "device.h"
#include "common/profiler.h"
#include "deviceAllocator.h"
#include "resource/shaderReflector.h"

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

        if(feature.rayTracing)
        {
            // Request Ray Tracing related features
            auto& asFeature = physicalDevice->requestFeatures<VkPhysicalDeviceAccelerationStructureFeaturesKHR>(
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT);
            asFeature.accelerationStructure = VK_TRUE;
            exts.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

            auto& rtPipelineFeature = physicalDevice->requestFeatures<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>(
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT);
            rtPipelineFeature.rayTracingPipeline = VK_TRUE;
            exts.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

            auto& rayQueryFeature = physicalDevice->requestFeatures<VkPhysicalDeviceRayQueryFeaturesKHR>(
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT);
            rayQueryFeature.rayQuery = VK_TRUE;
            exts.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        }

        if(feature.rayTracing)
        {
            // Request Multi-Draw Features EXT
            auto& multiDrawFeature = physicalDevice->requestFeatures<VkPhysicalDeviceMultiDrawFeaturesEXT>(
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT);
            multiDrawFeature.multiDraw = VK_TRUE;
            exts.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
            exts.push_back(VK_EXT_MULTI_DRAW_EXTENSION_NAME);
        }


        // must support features
        exts.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
        exts.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
        exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        exts.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        exts.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        exts.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
        exts.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
        exts.push_back(VK_KHR_PIPELINE_BINARY_EXTENSION_NAME);
        exts.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
    }

    // verify feature support
    {
        const auto& requiredFeature = createInfo.enabledFeatures;
        const auto& supportFeature  = physicalDevice->getSettings().feature;

        if(requiredFeature.rayTracing && !supportFeature.rayTracing)
        {
            CM_LOG_ERR("Ray Tracing feature not supported!");
            APH_ASSERT(false);
        }
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

    auto& extDynamicState3 = physicalDevice->requestFeatures<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT);
    extDynamicState3.extendedDynamicState3ColorBlendEquation = VK_TRUE;

    auto& shaderObjectFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceShaderObjectFeaturesEXT>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT);
    shaderObjectFeatures.shaderObject = VK_TRUE;

    auto& maintence5 = physicalDevice->requestFeatures<VkPhysicalDeviceMaintenance5FeaturesKHR>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR);
    maintence5.maintenance5 = VK_TRUE;

    auto& pipelineBinary = physicalDevice->requestFeatures<VkPhysicalDevicePipelineBinaryFeaturesKHR>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_FEATURES_KHR);
    pipelineBinary.pipelineBinaries = VK_TRUE;

    auto& sync2Features = physicalDevice->requestFeatures<VkPhysicalDeviceSynchronization2Features>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES);
    sync2Features.synchronization2 = VK_TRUE;

    auto& timelineSemaphoreFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceTimelineSemaphoreFeatures>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES);
    timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

    auto& descriptorBufferFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceDescriptorBufferFeaturesEXT>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT);
    descriptorBufferFeatures.descriptorBuffer                = VK_TRUE;
    descriptorBufferFeatures.descriptorBufferPushDescriptors = VK_TRUE;

    auto& maintenance4Features = physicalDevice->requestFeatures<VkPhysicalDeviceMaintenance4Features>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES);
    maintenance4Features.maintenance4 = VK_TRUE;

    auto& descriptorIndexingFeatures = physicalDevice->requestFeatures<VkPhysicalDeviceDescriptorIndexingFeatures>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES);
    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound           = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount  = VK_TRUE;
    descriptorIndexingFeatures.runtimeDescriptorArray                    = VK_TRUE;

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
    _VR(utils::setDebugObjectName(handle, VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(handle), "Device"));

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
                device->m_queues[queueType].push_back(device->m_resourcePool.queue.allocate(
                    device.get(), queue, queueFamilyIndex, queueIndex, queueFamilyProperties[queueFamilyIndex]));
            }
        }
    }

    // Return success.
    return device;
}

void Device::Destroy(Device* pDevice)
{
    APH_PROFILER_SCOPE();
    pDevice->waitIdle();
    // TODO
    delete pDevice->m_resourcePool.gpu;

    pDevice->m_resourcePool.pipeline.clear();
    pDevice->m_resourcePool.program.clear();
    pDevice->m_resourcePool.syncPrimitive.clear();
    pDevice->m_resourcePool.commandPool.clear();
    pDevice->m_resourcePool.setLayout.clear();
    pDevice->m_resourcePool.shader.clear();

    APH_ASSERT(pDevice->m_handle);
    pDevice->m_table.vkDestroyDevice(pDevice->m_handle, gVkAllocator);
}

VkFormat Device::getDepthFormat() const
{
    APH_PROFILER_SCOPE();
    return m_physicalDevice->findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

Result Device::create(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppLayout,
                      std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    VkSampler                                 vkImmutableSamplers[VULKAN_NUM_BINDINGS] = {};
    SmallVector<VkDescriptorSetLayoutBinding> vkBindings;

    const auto&                       pImmutableSamplers = createInfo.pImmutableSamplers;
    const auto&                       layout             = createInfo.setInfo.shaderLayout;
    const auto&                       stageForBinds      = createInfo.setInfo.stagesForBindings;
    SmallVector<VkDescriptorPoolSize> poolSizes;

    for(unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
    {
        uint32_t stages = stageForBinds[i];
        if(stages == 0)
        {
            continue;
        }

        unsigned arraySize = layout.arraySize[i];
        unsigned poolArraySize;
        if(arraySize == vk::ShaderLayout::UNSIZED_ARRAY)
        {
            arraySize     = VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
            poolArraySize = arraySize;
        }
        else
        {
            poolArraySize = arraySize * VULKAN_NUM_SETS_PER_POOL;
        }

        unsigned types = 0;
        if(layout.sampledImageMask & (1u << i))
        {
            if((layout.immutableSamplerMask & (1u << i)) && pImmutableSamplers && pImmutableSamplers[i])
            {
                vkImmutableSamplers[i] = pImmutableSamplers[i]->getHandle();
            }

            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, arraySize, stages,
                                  vkImmutableSamplers[i] != VK_NULL_HANDLE ? &vkImmutableSamplers[i] : nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolArraySize});
            types++;
        }

        if(layout.sampledTexelBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, poolArraySize});
            types++;
        }

        if(layout.storageTexelBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, poolArraySize});
            types++;
        }

        if(layout.storageImageMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, poolArraySize});
            types++;
        }

        if(layout.uniformBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolArraySize});
            types++;
        }

        if(layout.storageBufferMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, poolArraySize});
            types++;
        }

        if(layout.inputAttachmentMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, poolArraySize});
            types++;
        }

        if(layout.separateImageMask & (1u << i))
        {
            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, arraySize, stages, nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, poolArraySize});
            types++;
        }

        if(layout.samplerMask & (1u << i))
        {
            if((layout.immutableSamplerMask & (1u << i)) && pImmutableSamplers && pImmutableSamplers[i])
                vkImmutableSamplers[i] = pImmutableSamplers[i]->getHandle();

            vkBindings.push_back({i, VK_DESCRIPTOR_TYPE_SAMPLER, arraySize, stages,
                                  vkImmutableSamplers[i] != VK_NULL_HANDLE ? &vkImmutableSamplers[i] : nullptr});
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, poolArraySize});
            types++;
        }

        (void)types;
        APH_ASSERT(types <= 1 && "Descriptor set aliasing!");
    }

    VkDescriptorSetLayoutCreateInfo vkCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    vkCreateInfo.bindingCount                    = vkBindings.size();
    vkCreateInfo.pBindings                       = vkBindings.data();

    VkDescriptorSetLayout vkSetLayout;
    _VR(getDeviceTable()->vkCreateDescriptorSetLayout(getHandle(), &vkCreateInfo, vk::vkAllocator(), &vkSetLayout));
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                  reinterpret_cast<uint64_t>(vkSetLayout), debugName))

    *ppLayout = m_resourcePool.setLayout.allocate(this, createInfo, vkSetLayout, poolSizes, vkBindings);
    return Result::Success;
}

Result Device::create(const ShaderCreateInfo& createInfo, Shader** ppShader, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    const auto&              spv = createInfo.code;
    VkShaderModuleCreateInfo vkCreateInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv.size() * sizeof(spv[0]),
        .pCode    = spv.data(),
    };
    VkShaderModule handle = VK_NULL_HANDLE;
    if (createInfo.compile)
    {
        _VR(getDeviceTable()->vkCreateShaderModule(getHandle(), &vkCreateInfo, vk::vkAllocator(), &handle));
        _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(handle),
                                    debugName))
    }
    *ppShader = m_resourcePool.shader.allocate(createInfo, handle, reflectLayout(spv));
    return Result::Success;
}

Result Device::create(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram, std::string_view debugName)
{
    APH_PROFILER_SCOPE();
    CombinedResourceLayout combineLayout;
    std::vector<Shader*> shaders{};
    switch(createInfo.type)
    {
    case PipelineType::Geometry:
    {
        auto vs = createInfo.geometry.pVertex;
        auto fs = createInfo.geometry.pFragment;
        APH_ASSERT(vs);
        APH_ASSERT(fs);

        shaders = {vs, fs};
    }
    break;
    case PipelineType::Mesh:
    {
        auto ms = createInfo.mesh.pMesh;
        auto ts = createInfo.mesh.pTask;
        auto fs = createInfo.mesh.pFragment;
        APH_ASSERT(ms);
        APH_ASSERT(ts);
        APH_ASSERT(fs);
        shaders.push_back(ms);
        if(ts)
        {
            shaders.push_back(ts);
        }
        shaders.push_back(fs);
    }
    break;
    case PipelineType::Compute:
    {
        auto cs = createInfo.compute.pCompute;
        APH_ASSERT(cs);
        shaders = {cs};
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
    combineLayout = aph::combineLayout(shaders, &createInfo.samplerBank);

    // setup descriptor set layouts and pipeline layouts
    SmallVector<DescriptorSetLayout*> setLayouts = {};
    SmallVector<VkDescriptorSetLayout> vkSetLayouts;
    VkPipelineLayout                  pipelineLayout;
    auto                              samplerBank = &createInfo.samplerBank;
    {
        setLayouts.resize(VULKAN_NUM_DESCRIPTOR_SETS);

        unsigned numSets = 0;
        for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            DescriptorSetLayoutCreateInfo setLayoutCreateInfo{.setInfo            = combineLayout.setInfos[i],
                                                              .pImmutableSamplers = samplerBank->samplers[i]};
            APH_VR(create(setLayoutCreateInfo, &setLayouts[i]));
            if(combineLayout.descriptorSetMask.to_ulong() & (1u << i))
            {
                numSets = i + 1;
            }
        }

        if(numSets > getPhysicalDevice()->getProperties().limits.maxBoundDescriptorSets)
        {
            VK_LOG_ERR("Number of sets %u exceeds device limit of %u.", numSets,
                       getPhysicalDevice()->getProperties().limits.maxBoundDescriptorSets);
        }

        VkPipelineLayoutCreateInfo         info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        if(numSets)
        {
            vkSetLayouts.reserve(setLayouts.size());
            for(const auto& setLayout : setLayouts)
            {
                vkSetLayouts.push_back(setLayout->getHandle());
            }
            info.setLayoutCount = numSets;
            info.pSetLayouts    = vkSetLayouts.data();
        }

        if(combineLayout.pushConstantRange.stageFlags != 0)
        {
            info.pushConstantRangeCount = 1;
            info.pPushConstantRanges    = &combineLayout.pushConstantRange;
        }

        if(getDeviceTable()->vkCreatePipelineLayout(getHandle(), &info, vkAllocator(), &pipelineLayout) != VK_SUCCESS)
            VK_LOG_ERR("Failed to create pipeline layout.");
        _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(pipelineLayout),
                                      debugName))
    }

    HashMap<ShaderStage, VkShaderEXT> shaderObjectMaps;
    // setup shader object
    {
        SmallVector<VkShaderCreateInfoEXT> shaderCreateInfos;
        for (auto iter = shaders.cbegin(); iter != shaders.cend(); ++iter)
        {
            auto shader = *iter;
            VkShaderStageFlags nextStage = 0;
            if (auto nextIter = std::next(iter); nextIter != shaders.cend())
            {
                nextStage = utils::VkCast((*nextIter)->getStage());
            }
            VkShaderCreateInfoEXT soCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT,
                .stage = utils::VkCast(shader->getStage()),
                .nextStage = nextStage,
                // TODO binary support
                .codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
                .codeSize = shader->getCode().size() * sizeof(shader->getCode()[0]),
                .pCode = shader->getCode().data(),
                .pName = shader->getEntryPointName().data(),
                .setLayoutCount = static_cast<uint32_t>(vkSetLayouts.size()),
                .pSetLayouts = vkSetLayouts.data(),
            };

            if(combineLayout.pushConstantRange.stageFlags != 0)
            {
                soCreateInfo.pushConstantRangeCount = 1;
                soCreateInfo.pPushConstantRanges    = &combineLayout.pushConstantRange;
            }

            shaderCreateInfos.push_back(soCreateInfo);
        }

        SmallVector<VkShaderEXT> shaderObjects(shaderCreateInfos.size());
        _VR(m_table.vkCreateShadersEXT(getHandle(), shaderCreateInfos.size(), shaderCreateInfos.data(), vkAllocator(), shaderObjects.data()));

        for (size_t idx = 0; idx < shaders.size(); ++idx)
        {
            _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_SHADER_EXT, reinterpret_cast<uint64_t>(shaderObjects[idx]),
                                        debugName))
            shaderObjectMaps[shaders[idx]->getStage()] = shaderObjects[idx];
        }
    }

    *ppProgram = m_resourcePool.program.allocate(createInfo, combineLayout, pipelineLayout, setLayouts, shaderObjectMaps);

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
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image), debugName))
    *ppImage = m_resourcePool.image.allocate(this, createInfo, image);
    m_resourcePool.gpu->allocate(*ppImage);

    return Result::Success;
}

void Device::destroy(DescriptorSetLayout* pSetLayout)
{
    APH_PROFILER_SCOPE();
    getDeviceTable()->vkDestroyDescriptorSetLayout(getHandle(), pSetLayout->getHandle(), vkAllocator());
    m_resourcePool.setLayout.free(pSetLayout);
}

void Device::destroy(Shader* pShader)
{
    APH_PROFILER_SCOPE();
    getDeviceTable()->vkDestroyShaderModule(getHandle(), pShader->getHandle(), vk::vkAllocator());
    m_resourcePool.shader.free(pShader);
}

void Device::destroy(ShaderProgram* pProgram)
{
    APH_PROFILER_SCOPE();

    for(auto* setLayout : pProgram->m_pSetLayouts)
    {
        destroy(setLayout);
    }

    for (auto [_, shaderObject] : pProgram->m_shaderObjects)
    {
        getDeviceTable()->vkDestroyShaderEXT(getHandle(), shaderObject, vk::vkAllocator());
    }

    getDeviceTable()->vkDestroyPipelineLayout(getHandle(), pProgram->m_pipeLayout, vkAllocator());
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
    APH_ASSERT(pPipeline);
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
    VkCommandPoolCreateInfo cmdPoolInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = info.queue->getFamilyIndex(),
    };

    cmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VkCommandPool pool;
    _VR(getDeviceTable()->vkCreateCommandPool(getHandle(), &cmdPoolInfo, vkAllocator(), &pool));
    auto* pPool = new CommandPool{this, info, pool};
    return pPool;
}
Result Device::releaseCommandPool(CommandPool* pPool)
{
    APH_PROFILER_SCOPE();
    pPool->reset(true);
    getDeviceTable()->vkDestroyCommandPool(getHandle(), pPool->getHandle(), vkAllocator());
    delete pPool;
    return Result::Success;
}
void Device::executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func,
                                   const std::vector<Semaphore*>& waitSems, const std::vector<Semaphore*>& signalSems,
                                   Fence* pFence)
{
    APH_PROFILER_SCOPE();

    auto           commandPool = acquireCommandPool({.queue = queue, .transient = true});
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
        APH_VR(queue->submit({submitInfo}, pFence));
        // TODO async with caller
        pFence->wait();
    }

    APH_VR(releaseCommandPool(commandPool));
}
VkPipelineStageFlags Device::determinePipelineStageFlags(VkAccessFlags accessFlags, QueueType queueType)
{
    APH_PROFILER_SCOPE();
    VkPipelineStageFlags flags = 0;

    const auto& features = getCreateInfo().enabledFeatures;

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
            if(features.tessellationSupported)
            {
                flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
            }
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            if(features.rayTracing)
            {
                flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
            }
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
