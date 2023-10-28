#include "device.h"

const VkAllocationCallbacks* gVkAllocator = aph::vk::vkAllocator();

namespace aph::vk
{

#ifdef _VR
    #undef _VR
    #define _VR(f) \
        { \
            VkResult res = (f); \
            if(res != VK_SUCCESS) \
            { \
                APH_ASSERT(false); \
                VK_LOG_ERR("Check Result Failed."); \
            } \
        }
#endif

Device::Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle) :
    ResourceHandle(handle, createInfo),
    m_physicalDevice(pPhysicalDevice),
    m_resourcePool(this)
{
}

std::unique_ptr<Device> Device::Create(const DeviceCreateInfo& createInfo)
{
    PhysicalDevice* physicalDevice = createInfo.pPhysicalDevice;

    auto queueFamilyProperties = physicalDevice->m_queueFamilyProperties;
    auto queueFamilyCount      = queueFamilyProperties.size();

    // Allocate handles for all available queues.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilyCount);
    std::vector<std::vector<float>>      priorities(queueFamilyCount);
    for(auto i = 0U; i < queueFamilyCount; ++i)
    {
        const float defaultPriority = 1.0f;
        priorities[i].resize(queueFamilyProperties[i].queueCount, defaultPriority);
        queueCreateInfos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = i;
        queueCreateInfos[i].queueCount       = queueFamilyProperties[i].queueCount;
        queueCreateInfos[i].pQueuePriorities = priorities[i].data();
    }

    // Enable all physical device available features.
    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(physicalDevice->getHandle(), &supportedFeatures);
    VkPhysicalDeviceFeatures2 supportedFeatures2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    vkGetPhysicalDeviceFeatures2(physicalDevice->getHandle(), &supportedFeatures2);

    // TODO manage features
    supportedFeatures.sampleRateShading = VK_TRUE;
    supportedFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features sync2Features{
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = VK_TRUE,
    };

    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{
        .sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
        .pNext             = &sync2Features,
        .timelineSemaphore = VK_TRUE,
    };
    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeatures{
        .sType                           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .pNext                           = &timelineSemaphoreFeatures,
        .descriptorBuffer                = VK_TRUE,
        .descriptorBufferPushDescriptors = VK_TRUE,
    };

    VkPhysicalDeviceMaintenance4Features maintenance4Features{
        .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
        .pNext        = &descriptorBufferFeatures,
        .maintenance4 = VK_TRUE,
    };

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{
        .sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext                                     = &maintenance4Features,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound           = VK_TRUE,
        .descriptorBindingVariableDescriptorCount  = VK_TRUE,
        .runtimeDescriptorArray                    = VK_TRUE,
    };

    VkPhysicalDeviceInlineUniformBlockFeaturesEXT inlineUniformBlockFeature{
        .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES,
        .pNext              = &descriptorIndexingFeatures,
        .inlineUniformBlock = VK_TRUE,
    };

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext            = &inlineUniformBlockFeature,
        .dynamicRendering = VK_TRUE,
    };

    supportedFeatures2.pNext    = &dynamicRenderingFeature;
    supportedFeatures2.features = supportedFeatures;

    // Create the Vulkan device.
    VkDeviceCreateInfo deviceCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &supportedFeatures2,
        .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(createInfo.enabledExtensions.size()),
        .ppEnabledExtensionNames = createInfo.enabledExtensions.data(),
    };

    VkDevice handle = VK_NULL_HANDLE;
    _VR(vkCreateDevice(physicalDevice->getHandle(), &deviceCreateInfo, gVkAllocator, &handle));
    _VR(utils::setDebugObjectName(handle, VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(handle), "Device"))

    // Initialize Device class.
    auto device = std::unique_ptr<Device>(new Device(createInfo, physicalDevice, handle));
    volkLoadDeviceTable(&device->m_table, handle);
    device->m_supportedFeatures = std::move(supportedFeatures);

    // Get handles to all of the previously enumerated and created queues.
    device->m_queues.resize(queueFamilyCount);
    for(auto queueFamilyIndex = 0U; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
    {
        device->m_queues[queueFamilyIndex].resize(queueCreateInfos[queueFamilyIndex].queueCount);
        for(auto queueIndex = 0U; queueIndex < queueCreateInfos[queueFamilyIndex].queueCount; ++queueIndex)
        {
            VkQueue queue = VK_NULL_HANDLE;
            device->m_table.vkGetDeviceQueue(handle, queueFamilyIndex, queueIndex, &queue);
            device->m_queues[queueFamilyIndex][queueIndex] =
                std::make_unique<Queue>(queue, queueFamilyIndex, queueIndex, queueFamilyProperties[queueFamilyIndex]);
        }
    }

    // Return success.
    return device;
}

void Device::Destroy(Device* pDevice)
{
    pDevice->m_resourcePool.syncPrimitive.clear();

    for(auto& [_, commandpool] : pDevice->m_commandPools)
    {
        pDevice->m_table.vkDestroyCommandPool(pDevice->m_handle, commandpool, gVkAllocator);
    }

    if(pDevice->m_handle)
    {
        pDevice->m_table.vkDestroyDevice(pDevice->m_handle, gVkAllocator);
    }
}

VkFormat Device::getDepthFormat() const
{
    return m_physicalDevice->findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

Result Device::create(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, std::string_view debugName)
{
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
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = createInfo.size,
        .usage       = createInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer buffer;
    _VR(m_table.vkCreateBuffer(getHandle(), &bufferInfo, gVkAllocator, &buffer));
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer), debugName))

    VkMemoryDedicatedRequirementsKHR dedicatedRequirements = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
        nullptr,
    };

    VkMemoryRequirements2 memRequirements{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &dedicatedRequirements};
    const VkBufferMemoryRequirementsInfo2 bufferRequirementsInfo{VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
                                                                 nullptr, buffer};

    // create memory
    vkGetBufferMemoryRequirements2(m_handle, &bufferRequirementsInfo, &memRequirements);

    VkDeviceMemory memory;
    if(dedicatedRequirements.prefersDedicatedAllocation)
    {
        // Allocate memory with VkMemoryDedicatedAllocateInfoKHR::image
        // pointing to the image we are allocating the memory for
        VkMemoryDedicatedAllocateInfo dedicatedInfo{
            VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
            nullptr,
            VK_NULL_HANDLE,
            buffer,
        };

        VkMemoryAllocateInfo memoryAllocateInfo{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            &dedicatedInfo,
            memRequirements.memoryRequirements.size,
            m_physicalDevice->findMemoryType(createInfo.domain, memRequirements.memoryRequirements.memoryTypeBits),
        };

        _VR(m_table.vkAllocateMemory(getHandle(), &memoryAllocateInfo, gVkAllocator, &memory));
    }
    else
    {
        VkMemoryAllocateInfo allocInfo = init::memoryAllocateInfo(
            memRequirements.memoryRequirements.size,
            m_physicalDevice->findMemoryType(createInfo.domain, memRequirements.memoryRequirements.memoryTypeBits));
        _VR(m_table.vkAllocateMemory(m_handle, &allocInfo, gVkAllocator, &memory));
    }

    *ppBuffer = m_resourcePool.buffer.allocate(createInfo, buffer, memory);

    // bind buffer and memory
    _VR(m_table.vkBindBufferMemory(getHandle(), (*ppBuffer)->getHandle(), (*ppBuffer)->getMemory(), 0));

    return Result::Success;
}

Result Device::create(const ImageCreateInfo& createInfo, Image** ppImage, std::string_view debugName)
{
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
    _VR(m_table.vkCreateImage(m_handle, &imageCreateInfo, gVkAllocator, &image));
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(image), debugName))

    VkMemoryDedicatedRequirementsKHR dedicatedRequirements = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
        nullptr,
    };

    VkMemoryRequirements2 memRequirements{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &dedicatedRequirements};
    const VkImageMemoryRequirementsInfo2 imageRequirementsInfo{VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                                                               nullptr,  // pNext
                                                               image};
    vkGetImageMemoryRequirements2(m_handle, &imageRequirementsInfo, &memRequirements);

    VkDeviceMemory memory;
    if(dedicatedRequirements.prefersDedicatedAllocation)
    {
        // Allocate memory with VkMemoryDedicatedAllocateInfoKHR::image
        // pointing to the image we are allocating the memory for
        VkMemoryDedicatedAllocateInfo dedicatedInfo{
            VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
            nullptr,
            image,
            VK_NULL_HANDLE,
        };

        VkMemoryAllocateInfo memoryAllocateInfo{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            &dedicatedInfo,
            memRequirements.memoryRequirements.size,
            m_physicalDevice->findMemoryType(createInfo.domain, memRequirements.memoryRequirements.memoryTypeBits),
        };

        _VR(m_table.vkAllocateMemory(getHandle(), &memoryAllocateInfo, gVkAllocator, &memory));
    }
    else
    {
        VkMemoryAllocateInfo allocInfo{
            .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.memoryRequirements.size,
            .memoryTypeIndex =
                m_physicalDevice->findMemoryType(createInfo.domain, memRequirements.memoryRequirements.memoryTypeBits),
        };

        _VR(m_table.vkAllocateMemory(m_handle, &allocInfo, gVkAllocator, &memory));
    }

    *ppImage = m_resourcePool.image.allocate(this, createInfo, image, memory);

    if((*ppImage)->getMemory() != VK_NULL_HANDLE)
    {
        _VR(m_table.vkBindImageMemory(getHandle(), (*ppImage)->getHandle(), (*ppImage)->getMemory(), 0));
    }

    return Result::Success;
}

void Device::destroy(Buffer* pBuffer)
{
    if(pBuffer->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_handle, pBuffer->getMemory(), gVkAllocator);
    }
    m_table.vkDestroyBuffer(m_handle, pBuffer->getHandle(), gVkAllocator);
    m_resourcePool.buffer.free(pBuffer);
}

void Device::destroy(Image* pImage)
{
    if(pImage->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_handle, pImage->getMemory(), gVkAllocator);
    }
    m_table.vkDestroyImage(m_handle, pImage->getHandle(), gVkAllocator);
    m_resourcePool.image.free(pImage);
}

void Device::destroy(ImageView* pImageView)
{
    m_table.vkDestroyImageView(m_handle, pImageView->getHandle(), gVkAllocator);
    m_resourcePool.imageView.free(pImageView);
}

Result Device::create(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain, std::string_view debugName)
{
    *ppSwapchain = new SwapChain(createInfo, this);
    return Result::Success;
}

void Device::destroy(SwapChain* pSwapchain)
{
    m_table.vkDestroySwapchainKHR(getHandle(), pSwapchain->getHandle(), gVkAllocator);
    delete pSwapchain;
    pSwapchain = nullptr;
}

Queue* Device::getQueueByFlags(QueueType flags, uint32_t queueIndex)
{
    std::vector<uint32_t> supportedQueueFamilyIndexList = m_physicalDevice->getQueueFamilyIndexByFlags(flags);
    if(supportedQueueFamilyIndexList.empty())
    {
        return nullptr;
    }
    return m_queues[supportedQueueFamilyIndexList[0]][queueIndex].get();
}

void Device::waitIdle()
{
    m_table.vkDeviceWaitIdle(getHandle());
}

VkCommandPool Device::getCommandPoolWithQueue(Queue* queue)
{
    auto queueIndices = queue->getFamilyIndex();

    if(m_commandPools.contains(queueIndices))
    {
        return m_commandPools.at(queueIndices);
    }

    VkCommandPool pool = nullptr;
    {
        CommandPoolCreateInfo   createInfo{.queue = queue};
        VkCommandPoolCreateInfo cmdPoolInfo{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = createInfo.queue->getFamilyIndex(),
        };

        if(createInfo.transient)
        {
            cmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        }

        _VR(m_table.vkCreateCommandPool(m_handle, &cmdPoolInfo, gVkAllocator, &pool));
    }
    m_commandPools[queueIndices] = pool;
    return pool;
}

Result Device::allocateCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, Queue* pQueue)
{
    Queue*        queue = pQueue;
    VkCommandPool pool  = getCommandPoolWithQueue(queue);

    std::vector<VkCommandBuffer> handles(commandBufferCount);

    // Allocate a new command buffer.
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = commandBufferCount,
    };
    _VR(m_table.vkAllocateCommandBuffers(getHandle(), &allocInfo, handles.data()));

    for(auto i = 0; i < commandBufferCount; i++)
    {
        ppCommandBuffers[i] = new CommandBuffer(this, pool, handles[i], queue);
    }
    return Result::Success;
}

void Device::freeCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers)
{
    // Destroy all of the command buffers.
    for(auto i = 0U; i < commandBufferCount; ++i)
    {
        if(ppCommandBuffers != nullptr)
        {
            delete ppCommandBuffers[i];
            ppCommandBuffers = nullptr;
        }
    }
}

Result Device::create(const GraphicsPipelineCreateInfo& createInfo, Pipeline** ppPipeline, std::string_view debugName)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    APH_ASSERT(createInfo.pVertex && createInfo.pFragment);

    shaderStages.push_back(
        init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, createInfo.pVertex->getHandle()));
    shaderStages.push_back(
        init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, createInfo.pFragment->getHandle()));

    auto program =
        m_resourcePool.program.allocate(this, createInfo.pVertex, createInfo.pFragment, createInfo.pSamplerBank);

    // create rps
    RenderPipelineState rps    = {.createInfo = createInfo};
    const VertexInput&  vstate = rps.createInfo.vertexInput;
    rps.vkAttributes.resize(vstate.attributes.size());
    std::vector<bool> bufferAlreadyBound(vstate.bindings.size());

    for(uint32_t i = 0; i != rps.vkAttributes.size(); i++)
    {
        const auto& attr = vstate.attributes[i];

        rps.vkAttributes[i] = {.location = attr.location,
                               .binding  = attr.binding,
                               .format   = utils::VkCast(attr.format),
                               .offset   = (uint32_t)attr.offset};

        if(!bufferAlreadyBound[attr.binding])
        {
            bufferAlreadyBound[attr.binding] = true;
            rps.vkBindings.push_back({.binding   = attr.binding,
                                      .stride    = vstate.bindings[attr.binding].stride,
                                      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});
        }
    }

    const uint32_t numColorAttachments = createInfo.getNumColorAttachments();

    // Not all attachments are valid. We need to create color blend attachments only for active attachments
    VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[APH_MAX_COLOR_ATTACHMENTS] = {};
    VkFormat                            colorAttachmentFormats[APH_MAX_COLOR_ATTACHMENTS]     = {};

    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        const auto& attachment = createInfo.color[i];
        APH_ASSERT(attachment.format != Format::Undefined);
        colorAttachmentFormats[i] = utils::VkCast(attachment.format);
        if(!attachment.blendEnabled)
        {
            colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable         = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
        }
        else
        {
            colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable         = VK_TRUE,
                .srcColorBlendFactor = attachment.srcRGBBlendFactor,
                .dstColorBlendFactor = attachment.dstRGBBlendFactor,
                .colorBlendOp        = attachment.rgbBlendOp,
                .srcAlphaBlendFactor = attachment.srcAlphaBlendFactor,
                .dstAlphaBlendFactor = attachment.dstAlphaBlendFactor,
                .alphaBlendOp        = attachment.alphaBlendOp,
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
        }
    }

    const VkPipelineVertexInputStateCreateInfo ciVertexInputState = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = static_cast<uint32_t>(rps.vkBindings.size()),
        .pVertexBindingDescriptions      = !rps.vkBindings.empty() ? rps.vkBindings.data() : nullptr,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(rps.vkAttributes.size()),
        .pVertexAttributeDescriptions    = !rps.vkAttributes.empty() ? rps.vkAttributes.data() : nullptr,
    };

    VkPipeline handle;

    VulkanPipelineBuilder()
        // from Vulkan 1.0
        .dynamicState(VK_DYNAMIC_STATE_VIEWPORT)
        .dynamicState(VK_DYNAMIC_STATE_SCISSOR)
        // .dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS)
        // .dynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS)
        // from Vulkan 1.3
        // .dynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE)
        // .dynamicState(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE)
        // .dynamicState(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP)
        .primitiveTopology(createInfo.topology)
        .depthBiasEnable(createInfo.dynamicState.depthBiasEnable)
        .rasterizationSamples(aph::vk::utils::getSampleCountFlags(createInfo.samplesCount))
        .polygonMode(createInfo.polygonMode)
        .stencilStateOps(VK_STENCIL_FACE_FRONT_BIT, createInfo.frontFaceStencil.stencilFailureOp,
                         createInfo.frontFaceStencil.depthStencilPassOp, createInfo.frontFaceStencil.depthFailureOp,
                         createInfo.frontFaceStencil.stencilCompareOp)
        .stencilStateOps(VK_STENCIL_FACE_BACK_BIT, createInfo.backFaceStencil.stencilFailureOp,
                         createInfo.backFaceStencil.depthStencilPassOp, createInfo.backFaceStencil.depthFailureOp,
                         createInfo.backFaceStencil.stencilCompareOp)
        .stencilMasks(VK_STENCIL_FACE_FRONT_BIT, 0xFF, createInfo.frontFaceStencil.writeMask,
                      createInfo.frontFaceStencil.readMask)
        .stencilMasks(VK_STENCIL_FACE_BACK_BIT, 0xFF, createInfo.backFaceStencil.writeMask,
                      createInfo.backFaceStencil.readMask)
        .shaderStage(shaderStages)
        .cullMode(createInfo.cullMode)
        .frontFace(createInfo.frontFaceWinding)
        .vertexInputState(ciVertexInputState)
        .colorAttachments(colorBlendAttachmentStates, colorAttachmentFormats, numColorAttachments)
        .depthAttachmentFormat(createInfo.depthFormat)
        .stencilAttachmentFormat(createInfo.stencilFormat)
        .build(this, VK_NULL_HANDLE, program->getPipelineLayout(), &handle, createInfo.debugName);

    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(handle), debugName))
    *ppPipeline = m_resourcePool.pipeline.allocate(this, rps, handle, program);

    return Result::Success;
}

void Device::destroy(Pipeline* pipeline)
{
    auto program = pipeline->getProgram();
    m_resourcePool.program.free(program);

    m_table.vkDestroyPipeline(getHandle(), pipeline->getHandle(), gVkAllocator);
    m_resourcePool.pipeline.free(pipeline);
}

Result Device::create(const ComputePipelineCreateInfo& createInfo, Pipeline** ppPipeline, std::string_view debugName)
{
    APH_ASSERT(createInfo.pCompute);
    auto program = m_resourcePool.program.allocate(this, createInfo.pCompute, createInfo.pSamplerBank);
    VkComputePipelineCreateInfo ci = init::computePipelineCreateInfo(program->getPipelineLayout());
    ci.stage                       = init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT,
                                                                         program->getShader(ShaderStage::CS)->getHandle());
    VkPipeline handle              = VK_NULL_HANDLE;
    _VR(m_table.vkCreateComputePipelines(this->getHandle(), VK_NULL_HANDLE, 1, &ci, gVkAllocator, &handle));
    _VR(utils::setDebugObjectName(getHandle(), VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(handle), debugName))
    *ppPipeline = m_resourcePool.pipeline.allocate(this, createInfo, handle, program);
    return Result::Success;
}

Result Device::waitForFence(const std::vector<VkFence>& fences, bool waitAll, uint32_t timeout)
{
    return utils::getResult(m_table.vkWaitForFences(getHandle(), fences.size(), fences.data(), VK_TRUE, UINT64_MAX));
}

Result Device::flushMemory(VkDeviceMemory memory, MemoryRange range)
{
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

Result Device::mapMemory(Buffer* pBuffer, void* mapped, MemoryRange range)
{
    if(range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }
    if(mapped == nullptr)
    {
        return utils::getResult(
            m_table.vkMapMemory(getHandle(), pBuffer->getMemory(), range.offset, range.size, 0, &pBuffer->getMapped()));
    }
    return utils::getResult(
        m_table.vkMapMemory(getHandle(), pBuffer->getMemory(), range.offset, range.size, 0, &mapped));
}

void Device::unMapMemory(Buffer* pBuffer)
{
    m_table.vkUnmapMemory(getHandle(), pBuffer->getMemory());
}

Result Device::create(const SamplerCreateInfo& createInfo, Sampler** ppSampler, std::string_view debugName)
{
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
        .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .magFilter        = createInfo.magFilter,
        .minFilter        = createInfo.minFilter,
        .mipmapMode       = createInfo.mipMapMode,
        .addressModeU     = createInfo.addressU,
        .addressModeV     = createInfo.addressV,
        .addressModeW     = createInfo.addressW,
        .mipLodBias       = createInfo.mipLodBias,
        .anisotropyEnable = (createInfo.maxAnisotropy > 0.0f && getFeatures().samplerAnisotropy) ? VK_TRUE : VK_FALSE,
        .maxAnisotropy    = createInfo.maxAnisotropy,
        .compareEnable    = createInfo.compareFunc != VK_COMPARE_OP_NEVER ? VK_TRUE : VK_FALSE,
        .compareOp        = createInfo.compareFunc,
        .minLod           = minSamplerLod,
        .maxLod           = maxSamplerLod,
        .borderColor      = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
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
    m_table.vkDestroySampler(getHandle(), pSampler->getHandle(), gVkAllocator);
    m_resourcePool.sampler.free(pSampler);
}

void Device::executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func)
{
    CommandBuffer* cmd = nullptr;
    APH_CHECK_RESULT(allocateCommandBuffers(1, &cmd, queue));

    _VR(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
    func(cmd);
    _VR(cmd->end());

    QueueSubmitInfo submitInfo{.commandBuffers = {cmd}};
    APH_CHECK_RESULT(queue->submit({submitInfo}, VK_NULL_HANDLE));
    APH_CHECK_RESULT(queue->waitIdle());

    freeCommandBuffers(1, &cmd);
}

}  // namespace aph::vk
