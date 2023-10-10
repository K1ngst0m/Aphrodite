#include "device.h"
#include "api/gpuResource.h"

namespace aph::vk
{

#define _VR(f) \
    { \
        VkResult res = (f); \
        if(res != VK_SUCCESS) \
        { \
            return res; \
        } \
    }

Device::Device(const DeviceCreateInfo& createInfo, PhysicalDevice* pPhysicalDevice, VkDevice handle) :
    m_physicalDevice(pPhysicalDevice)
{
    getHandle()     = handle;
    getCreateInfo() = createInfo;
}

VkResult Device::Create(const DeviceCreateInfo& createInfo, Device** ppDevice)
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
    _VR(vkCreateDevice(physicalDevice->getHandle(), &deviceCreateInfo, nullptr, &handle));

    // Initialize Device class.
    auto* device = new Device(createInfo, physicalDevice, handle);
    volkLoadDeviceTable(&device->m_table, handle);
    device->m_supportedFeatures = supportedFeatures;

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

    // Copy address of object instance.
    *ppDevice = device;

    // Return success.
    return VK_SUCCESS;
}

void Device::Destroy(Device* pDevice)
{
    for(auto commandPool : pDevice->m_threadCommandPools)
    {
        pDevice->destroyCommandPool(commandPool);
    }
    for(auto& [_, commandpool] : pDevice->m_commandPools)
    {
        pDevice->destroyCommandPool(commandpool);
    }

    if(pDevice->m_handle)
    {
        pDevice->m_table.vkDestroyDevice(pDevice->m_handle, nullptr);
    }
    delete pDevice;
    pDevice = nullptr;
}

VkResult Device::createCommandPool(const CommandPoolCreateInfo& createInfo, VkCommandPool* ppPool)
{
    VkCommandPoolCreateInfo cmdPoolInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = createInfo.queue->getFamilyIndex(),
    };

    if(createInfo.transient)
    {
        cmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    _VR(m_table.vkCreateCommandPool(m_handle, &cmdPoolInfo, nullptr, &cmdPool));
    *ppPool = cmdPool;
    return VK_SUCCESS;
}

VkFormat Device::getDepthFormat() const
{
    return m_physicalDevice->findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkResult Device::createImageView(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, Image* pImage)
{
    VkImageViewCreateInfo info{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .image    = pImage->getHandle(),
        .viewType = createInfo.viewType,
        .format   = createInfo.format,
    };
    info.subresourceRange = {
        .aspectMask     = utils::getImageAspect(createInfo.format),
        .baseMipLevel   = createInfo.subresourceRange.baseMipLevel,
        .levelCount     = createInfo.subresourceRange.levelCount,
        .baseArrayLayer = createInfo.subresourceRange.baseArrayLayer,
        .layerCount     = createInfo.subresourceRange.layerCount,
    };
    memcpy(&info.components, &createInfo.components, sizeof(VkComponentMapping));

    VkImageView handle = VK_NULL_HANDLE;
    _VR(m_table.vkCreateImageView(getHandle(), &info, nullptr, &handle));

    *ppImageView = new ImageView(createInfo, pImage, handle);

    return VK_SUCCESS;
}

VkResult Device::createBuffer(const BufferCreateInfo& createInfo, Buffer** ppBuffer, const void* data,
                              bool persistmentMap)
{
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = createInfo.size,
        .usage       = createInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer buffer;
    _VR(vkCreateBuffer(getHandle(), &bufferInfo, nullptr, &buffer));

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

        _VR(vkAllocateMemory(getHandle(), &memoryAllocateInfo, nullptr, &memory));
    }
    else
    {
        VkMemoryAllocateInfo allocInfo = init::memoryAllocateInfo(
            memRequirements.memoryRequirements.size,
            m_physicalDevice->findMemoryType(createInfo.domain, memRequirements.memoryRequirements.memoryTypeBits));
        _VR(vkAllocateMemory(m_handle, &allocInfo, nullptr, &memory));
    }

    *ppBuffer = new Buffer(createInfo, buffer, memory);

    // bind buffer and memory
    _VR(bindMemory(*ppBuffer));

    if(data)
    {
        mapMemory(*ppBuffer);
        (*ppBuffer)->write(data);
        if(!persistmentMap)
        {
            unMapMemory(*ppBuffer);
        }
    }

    return VK_SUCCESS;
}

VkResult Device::createImage(const ImageCreateInfo& createInfo, Image** ppImage)
{
    VkImageCreateInfo imageCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = createInfo.flags,
        .imageType     = static_cast<VkImageType>(createInfo.imageType),
        .format        = static_cast<VkFormat>(createInfo.format),
        .mipLevels     = createInfo.mipLevels,
        .arrayLayers   = createInfo.arrayLayers,
        .samples       = static_cast<VkSampleCountFlagBits>(createInfo.samples),
        .tiling        = static_cast<VkImageTiling>(createInfo.tiling),
        .usage         = createInfo.usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = static_cast<VkImageLayout>(createInfo.initialLayout),
    };

    imageCreateInfo.extent.width  = createInfo.extent.width;
    imageCreateInfo.extent.height = createInfo.extent.height;
    imageCreateInfo.extent.depth  = createInfo.extent.depth;

    VkImage image;
    _VR(vkCreateImage(m_handle, &imageCreateInfo, nullptr, &image));

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

        _VR(vkAllocateMemory(getHandle(), &memoryAllocateInfo, nullptr, &memory));
    }
    else
    {
        VkMemoryAllocateInfo allocInfo{
            .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.memoryRequirements.size,
            .memoryTypeIndex =
                m_physicalDevice->findMemoryType(createInfo.domain, memRequirements.memoryRequirements.memoryTypeBits),
        };

        _VR(vkAllocateMemory(m_handle, &allocInfo, nullptr, &memory));
    }

    *ppImage = new Image(this, createInfo, image, memory);

    if((*ppImage)->getMemory() != VK_NULL_HANDLE)
    {
        _VR(bindMemory(*ppImage));
    }

    return VK_SUCCESS;
}

PhysicalDevice* Device::getPhysicalDevice() const
{
    return m_physicalDevice;
}

void Device::destroyBuffer(Buffer* pBuffer)
{
    if(pBuffer->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_handle, pBuffer->getMemory(), nullptr);
    }
    vkDestroyBuffer(m_handle, pBuffer->getHandle(), nullptr);
    delete pBuffer;
    pBuffer = nullptr;
}

void Device::destroyImage(Image* pImage)
{
    if(pImage->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_handle, pImage->getMemory(), nullptr);
    }
    vkDestroyImage(m_handle, pImage->getHandle(), nullptr);
    delete pImage;
    pImage = nullptr;
}

void Device::destroyImageView(ImageView* pImageView)
{
    vkDestroyImageView(m_handle, pImageView->getHandle(), nullptr);
    delete pImageView;
    pImageView = nullptr;
}

VkResult Device::createSwapchain(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain)
{
    *ppSwapchain = new SwapChain(createInfo, this);
    return VK_SUCCESS;
}

void Device::destroySwapchain(SwapChain* pSwapchain)
{
    vkDestroySwapchainKHR(getHandle(), pSwapchain->getHandle(), nullptr);
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

VkResult Device::waitIdle()
{
    return vkDeviceWaitIdle(getHandle());
}

VkCommandPool Device::getCommandPoolWithQueue(Queue* queue)
{
    auto queueIndices = queue->getFamilyIndex();

    if(m_commandPools.count(queueIndices))
    {
        return m_commandPools.at(queueIndices);
    }

    CommandPoolCreateInfo createInfo{.queue = queue};
    VkCommandPool          pool = nullptr;
    createCommandPool(createInfo, &pool);
    m_commandPools[queueIndices] = pool;
    return pool;
}

void Device::destroyCommandPool(VkCommandPool pPool)
{
    vkDestroyCommandPool(getHandle(), pPool, nullptr);
    pPool = nullptr;
}

VkResult Device::allocateCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, Queue* pQueue)
{
    Queue* queue = pQueue;
    VkCommandPool  pool  = getCommandPoolWithQueue(queue);

    std::vector<VkCommandBuffer> handles(commandBufferCount);

    // Allocate a new command buffer.
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = commandBufferCount,
    };
    _VR(vkAllocateCommandBuffers(getHandle(), &allocInfo, handles.data()));

    for(auto i = 0; i < commandBufferCount; i++)
    {
        ppCommandBuffers[i] = new CommandBuffer(this, pool, handles[i], queue->getFamilyIndex());
    }
    return VK_SUCCESS;
}

void Device::freeCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers)
{
    // Destroy all of the command buffers.
    for(auto i = 0U; i < commandBufferCount; ++i)
    {
        delete ppCommandBuffers[i];
        ppCommandBuffers = nullptr;
    }
}

VkResult Device::createGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, Pipeline** ppPipeline)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for(const auto& [stage, sModule] : createInfo.pProgram->getShaders())
    {
        shaderStages.push_back(init::pipelineShaderStageCreateInfo(utils::VkCast(stage), sModule->getHandle()));
    }

    // create rps
    RenderPipelineState rps                                     = {.createInfo = createInfo};
    const VertexInput&  vstate                                  = rps.createInfo.vertexInput;
    rps.numAttributes                                           = vstate.getNumAttributes();
    bool bufferAlreadyBound[VertexInput::APH_VERTEX_BUFFER_MAX] = {};

    for(uint32_t i = 0; i != rps.numAttributes; i++)
    {
        const auto& attr = vstate.attributes[i];

        rps.vkAttributes[i] = {
            .location = attr.location, .binding = attr.binding, .format = attr.format, .offset = (uint32_t)attr.offset};

        if(!bufferAlreadyBound[attr.binding])
        {
            bufferAlreadyBound[attr.binding]  = true;
            rps.vkBindings[rps.numBindings++] = {.binding   = attr.binding,
                                                 .stride    = vstate.inputBindings[attr.binding].stride,
                                                 .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
        }
    }

    const uint32_t numColorAttachments = createInfo.getNumColorAttachments();

    // Not all attachments are valid. We need to create color blend attachments only for active attachments
    VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[APH_MAX_COLOR_ATTACHMENTS] = {};
    VkFormat                            colorAttachmentFormats[APH_MAX_COLOR_ATTACHMENTS]     = {};

    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        const auto& attachment = createInfo.color[i];
        APH_ASSERT(attachment.format != VK_FORMAT_UNDEFINED);
        colorAttachmentFormats[i] = attachment.format;
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
        .vertexBindingDescriptionCount   = rps.numBindings,
        .pVertexBindingDescriptions      = rps.numBindings ? rps.vkBindings : nullptr,
        .vertexAttributeDescriptionCount = rps.numAttributes,
        .pVertexAttributeDescriptions    = rps.numAttributes ? rps.vkAttributes : nullptr,
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
        .build(getHandle(), VK_NULL_HANDLE, createInfo.pProgram->getPipelineLayout(), &handle, createInfo.debugName);

    *ppPipeline = new Pipeline(this, rps, handle);

    return VK_SUCCESS;
}

void Device::destroyPipeline(Pipeline* pipeline)
{
    m_table.vkDestroyPipeline(getHandle(), pipeline->getHandle(), nullptr);
    delete pipeline;
    pipeline = nullptr;
}

VkResult Device::createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo,
                                           DescriptorSetLayout**                  ppDescriptorSetLayout)
{
    VkDescriptorSetLayout setLayout;
    _VR(m_table.vkCreateDescriptorSetLayout(m_handle, &createInfo, nullptr, &setLayout));
    *ppDescriptorSetLayout = new DescriptorSetLayout(this, createInfo, setLayout);
    return VK_SUCCESS;
}
VkResult Device::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindingds,
                                           DescriptorSetLayout**                            ppDescriptorSetLayout)
{
    VkDescriptorSetLayoutCreateInfo createInfo = init::descriptorSetLayoutCreateInfo(bindingds);
    return createDescriptorSetLayout(createInfo, ppDescriptorSetLayout);
}

void Device::destroyDescriptorSetLayout(DescriptorSetLayout* pLayout)
{
    m_table.vkDestroyDescriptorSetLayout(m_handle, pLayout->getHandle(), nullptr);
    delete pLayout;
}
VkResult Device::createComputePipeline(const ComputePipelineCreateInfo& createInfo, Pipeline** ppPipeline)
{
    VkComputePipelineCreateInfo ci = init::computePipelineCreateInfo(createInfo.pProgram->getPipelineLayout());
    ci.stage                       = init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT,
                                                                         createInfo.pProgram->getShader(ShaderStage::CS)->getHandle());
    VkPipeline handle              = VK_NULL_HANDLE;
    _VR(m_table.vkCreateComputePipelines(this->getHandle(), VK_NULL_HANDLE, 1, &ci, nullptr, &handle));
    *ppPipeline = new Pipeline(this, createInfo, handle);
    return VK_SUCCESS;
}

VkResult Device::waitForFence(const std::vector<VkFence>& fences, bool waitAll, uint32_t timeout)
{
    return m_table.vkWaitForFences(getHandle(), fences.size(), fences.data(), VK_TRUE, UINT64_MAX);
}

VkResult Device::createDeviceLocalBuffer(const BufferCreateInfo& createInfo, Buffer** ppBuffer, const void* data)
{
    // using staging buffer
    Buffer* stagingBuffer{};
    {
        BufferCreateInfo stagingCI{
            .size   = static_cast<uint32_t>(createInfo.size),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };
        _VR(createBuffer(stagingCI, &stagingBuffer, data));
    }

    Buffer* buffer = nullptr;
    {
        auto bufferCI   = createInfo;
        bufferCI.domain = BufferDomain::Device;
        bufferCI.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        _VR(createBuffer(bufferCI, &buffer));
    }

    executeSingleCommands(QueueType::GRAPHICS,
                          [&](CommandBuffer* cmd) { cmd->copyBuffer(stagingBuffer, buffer, createInfo.size); });
    *ppBuffer = buffer;
    destroyBuffer(stagingBuffer);
    return VK_SUCCESS;
};

VkResult Device::createDeviceLocalImage(const ImageCreateInfo& createInfo, Image** ppImage,
                                        const std::vector<uint8_t>& data)
{
    bool           genMipmap = createInfo.mipLevels > 1;
    const uint32_t width     = createInfo.extent.width;
    const uint32_t height    = createInfo.extent.height;

    // Load texture from image buffer
    Buffer* stagingBuffer;
    {
        BufferCreateInfo bufferCI{
            .size   = static_cast<uint32_t>(data.size()),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };
        createBuffer(bufferCI, &stagingBuffer, data.data());
    }

    Image* texture{};
    {
        auto imageCI = createInfo;
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.domain = ImageDomain::Device;
        if(genMipmap)
        {
            imageCI.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        _VR(createImage(imageCI, &texture));

        executeSingleCommands(QueueType::GRAPHICS, [&](CommandBuffer* cmd) {
            cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            cmd->copyBufferToImage(stagingBuffer, texture);
            if(genMipmap)
            {
                cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            }
        });

        executeSingleCommands(QueueType::GRAPHICS, [&](CommandBuffer* cmd) {
            if(genMipmap)
            {
                // generate mipmap chains
                for(int32_t i = 1; i < imageCI.mipLevels; i++)
                {
                    VkImageBlit imageBlit{};

                    // Source
                    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.srcSubresource.layerCount = 1;
                    imageBlit.srcSubresource.mipLevel   = i - 1;
                    imageBlit.srcOffsets[1].x           = int32_t(width >> (i - 1));
                    imageBlit.srcOffsets[1].y           = int32_t(height >> (i - 1));
                    imageBlit.srcOffsets[1].z           = 1;

                    // Destination
                    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.dstSubresource.layerCount = 1;
                    imageBlit.dstSubresource.mipLevel   = i;
                    imageBlit.dstOffsets[1].x           = int32_t(width >> i);
                    imageBlit.dstOffsets[1].y           = int32_t(height >> i);
                    imageBlit.dstOffsets[1].z           = 1;

                    VkImageSubresourceRange mipSubRange = {};
                    mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
                    mipSubRange.baseMipLevel            = i;
                    mipSubRange.levelCount              = 1;
                    mipSubRange.layerCount              = 1;

                    // Prepare current mip level as image blit destination
                    cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &mipSubRange,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                    // Blit from previous level
                    cmd->blitImage(texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
                    cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &mipSubRange,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                }

                cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
            else
            {
                cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        });
    }

    destroyBuffer(stagingBuffer);
    *ppImage = texture;

    return VK_SUCCESS;
}
VkResult Device::flushMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
{
    VkMappedMemoryRange mappedRange = {
        .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = memory,
        .offset = offset,
        .size   = size,
    };
    return m_table.vkFlushMappedMemoryRanges(getHandle(), 1, &mappedRange);
}
VkResult Device::invalidateMemory(VkDeviceMemory memory, VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {
        .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = memory,
        .offset = offset,
        .size   = size,
    };
    return m_table.vkInvalidateMappedMemoryRanges(getHandle(), 1, &mappedRange);
}

VkResult Device::mapMemory(Buffer* pBuffer, void* mapped, VkDeviceSize offset, VkDeviceSize size)
{
    if(mapped == nullptr)
    {
        return m_table.vkMapMemory(getHandle(), pBuffer->getMemory(), offset, size, 0, &pBuffer->getMapped());
    }
    return m_table.vkMapMemory(getHandle(), pBuffer->getMemory(), offset, size, 0, &mapped);
}

VkResult Device::bindMemory(Buffer* pBuffer, VkDeviceSize offset)
{
    return m_table.vkBindBufferMemory(getHandle(), pBuffer->getHandle(), pBuffer->getMemory(), offset);
}

VkResult Device::bindMemory(Image* pImage, VkDeviceSize offset)
{
    return m_table.vkBindImageMemory(getHandle(), pImage->getHandle(), pImage->getMemory(), offset);
}

void Device::unMapMemory(Buffer* pBuffer)
{
    m_table.vkUnmapMemory(getHandle(), pBuffer->getMemory());
}

VkResult Device::createCubeMap(const std::array<std::shared_ptr<ImageInfo>, 6>& images, Image** ppImage)
{
    uint32_t               cubeMapWidth{}, cubeMapHeight{};
    uint32_t               mipLevels = 0;
    std::array<Buffer*, 6> stagingBuffers;
    for(auto idx = 0; idx < 6; idx++)
    {
        const auto& image = images[idx];
        cubeMapWidth      = image->width;
        cubeMapHeight     = image->height;

        {
            BufferCreateInfo createInfo{
                .size   = static_cast<uint32_t>(image->data.size()),
                .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .domain = BufferDomain::Host,
            };

            createBuffer(createInfo, &stagingBuffers[idx]);
            mapMemory(stagingBuffers[idx]);
            stagingBuffers[idx]->write(image->data.data());
            unMapMemory(stagingBuffers[idx]);
        }
    }
    mipLevels = aph::utils::calculateFullMipLevels(cubeMapWidth, cubeMapHeight);

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    for(uint32_t face = 0; face < 6; face++)
    {
        // TODO mip levels
        auto level = 0;
        // for(uint32_t level = 0; level < mipLevels; level++)
        // {
        VkBufferImageCopy bufferCopyRegion               = {};
        bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel       = level;
        bufferCopyRegion.imageSubresource.baseArrayLayer = face;
        bufferCopyRegion.imageSubresource.layerCount     = 1;
        bufferCopyRegion.imageExtent.width               = cubeMapWidth >> level;
        bufferCopyRegion.imageExtent.height              = cubeMapHeight >> level;
        bufferCopyRegion.imageExtent.depth               = 1;
        bufferCopyRegion.bufferOffset                    = 0;
        bufferCopyRegions.push_back(bufferCopyRegion);
        // }
    }

    // Image barrier for optimal image (target)
    // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
    VkImageSubresourceRange subresourceRange = {
        .aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount   = mipLevels,
        .layerCount   = 6,
    };

    Image*          cubeMapImage{};
    ImageCreateInfo imageCI{
        .extent      = {cubeMapWidth, cubeMapHeight, 1},
        .flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .mipLevels   = mipLevels,
        .arrayLayers = 6,
        .usage       = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .domain      = ImageDomain::Device,
        .imageType   = VK_IMAGE_TYPE_2D,
        .format      = VK_FORMAT_R8G8B8A8_UNORM,
    };
    createImage(imageCI, &cubeMapImage);

    executeSingleCommands(QueueType::GRAPHICS, [&](CommandBuffer* pCommandBuffer) {
        pCommandBuffer->transitionImageLayout(cubeMapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &subresourceRange);
        // Copy the cube map faces from the staging buffer to the optimal tiled image
        for(uint32_t idx = 0; idx < 6; idx++)
        {
            pCommandBuffer->copyBufferToImage(stagingBuffers[idx], cubeMapImage, {bufferCopyRegions[idx]});
        }
        pCommandBuffer->transitionImageLayout(cubeMapImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                              &subresourceRange);
    });

    for(auto* buffer : stagingBuffers)
    {
        destroyBuffer(buffer);
    }

    *ppImage = cubeMapImage;
    return VK_SUCCESS;
}

VkResult Device::createSampler(const VkSamplerCreateInfo& createInfo, Sampler** ppSampler, bool immutable)
{
    VkSampler handle;
    _VR(m_table.vkCreateSampler(getHandle(), &createInfo, nullptr, &handle));
    *ppSampler = new Sampler(this, createInfo, handle, immutable);
    return VK_SUCCESS;
}

void Device::destroySampler(Sampler* pSampler)
{
    m_table.vkDestroySampler(getHandle(), pSampler->getHandle(), nullptr);
    delete pSampler;
    pSampler = nullptr;
}

VkResult Device::executeSingleCommands(Queue* queue, const std::function<void(CommandBuffer* pCmdBuffer)>&& func)
{
    CommandBuffer* cmd = nullptr;
    _VR(allocateCommandBuffers(1, &cmd, queue));

    _VR(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
    func(cmd);
    _VR(cmd->end());

    QueueSubmitInfo submitInfo{.commandBuffers = {cmd}};
    _VR(queue->submit({submitInfo}, VK_NULL_HANDLE));
    _VR(queue->waitIdle());

    freeCommandBuffers(1, &cmd);

    return VK_SUCCESS;
}

VkResult Device::executeSingleCommands(QueueType type, const std::function<void(CommandBuffer* pCmdBuffer)>&& func)
{
    auto* queue = getQueueByFlags(type);
    return executeSingleCommands(queue, std::forward<const std::function<void(CommandBuffer * pCmdBuffer)>>(func));
}
void Device::destroyShaderProgram(ShaderProgram* pProgram)
{
    delete pProgram;
    pProgram = nullptr;
}
VkResult Device::createShaderProgram(ShaderProgram** ppProgram, Shader* cs, const ImmutableSamplerBank* samplerBank)
{
    *ppProgram = new ShaderProgram(this, cs, samplerBank);
    return VK_SUCCESS;
}
VkResult Device::createShaderProgram(ShaderProgram** ppProgram, Shader* vs, Shader* fs,
                                     const ImmutableSamplerBank* samplerBank)
{
    *ppProgram = new ShaderProgram(this, vs, fs, samplerBank);
    return VK_SUCCESS;
}

VkResult Device::allocateThreadCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers,
                                              Queue* pQueue)
{
    auto                  queueIndices = pQueue->getFamilyIndex();
    CommandPoolCreateInfo createInfo{.queue = pQueue};

    for(auto i = 0; i < commandBufferCount; i++)
    {
        VkCommandPool pool{};
        createCommandPool(createInfo, &pool);
        std::vector<VkCommandBuffer> handles(commandBufferCount);

        // Allocate a new command buffer.
        VkCommandBufferAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = commandBufferCount,
        };
        _VR(vkAllocateCommandBuffers(getHandle(), &allocInfo, handles.data()));
        ppCommandBuffers[i] = new CommandBuffer(this, pool, handles[i], queueIndices);
        m_threadCommandPools.push_back(pool);
    }
    return VK_SUCCESS;
}

VkResult Device::resetCommandPool(VkCommandPool pPool)
{
    return m_table.vkResetCommandPool(getHandle(), pPool, 0);
}
}  // namespace aph::vk
