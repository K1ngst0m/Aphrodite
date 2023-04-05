#include "device.h"

namespace aph
{

#ifdef VK_CHECK_RESULT
#    undef VK_CHECK_RESULT
#endif

#define VK_CHECK_RESULT(f) \
    { \
        VkResult res = (f); \
        if(res != VK_SUCCESS) \
        { \
            return res; \
        } \
    }

VkResult VulkanDevice::Create(const DeviceCreateInfo &createInfo, VulkanDevice **ppDevice)
{
    VulkanPhysicalDevice *physicalDevice = createInfo.pPhysicalDevice;

    auto queueFamilyProperties = physicalDevice->getQueueFamilyProperties();
    auto queueFamilyCount = queueFamilyProperties.size();

    // Allocate handles for all available queues.
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilyCount);
    std::vector<std::vector<float>> priorities(queueFamilyCount);
    for(auto i = 0U; i < queueFamilyCount; ++i)
    {
        const float defaultPriority = 1.0f;
        priorities[i].resize(queueFamilyProperties[i].queueCount, defaultPriority);
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = i;
        queueCreateInfos[i].queueCount = queueFamilyProperties[i].queueCount;
        queueCreateInfos[i].pQueuePriorities = priorities[i].data();
    }

    // Enable all physical device available features.
    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(physicalDevice->getHandle(), &supportedFeatures);
    VkPhysicalDeviceFeatures2 supportedFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    vkGetPhysicalDeviceFeatures2(physicalDevice->getHandle(), &supportedFeatures2);

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };

	VkPhysicalDeviceInlineUniformBlockFeaturesEXT inlineUniformBlockFeature{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES,
        .pNext = &descriptorIndexingFeatures,
		.inlineUniformBlock = VK_TRUE,
    };

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext = &inlineUniformBlockFeature,
        .dynamicRendering = VK_TRUE,
    };

    supportedFeatures2.pNext = &dynamicRenderingFeature;
    supportedFeatures2.features = supportedFeatures;

    // Create the Vulkan device.
    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &supportedFeatures2,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(createInfo.enabledExtensions.size()),
        .ppEnabledExtensionNames = createInfo.enabledExtensions.data(),
    };

    VkDevice handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateDevice(physicalDevice->getHandle(), &deviceCreateInfo, nullptr, &handle));

    // Initialize Device class.
    auto *device = new VulkanDevice();
    device->m_handle = handle;
    device->m_createInfo = createInfo;
    device->m_physicalDevice = physicalDevice;

    // Get handles to all of the previously enumerated and created queues.
    device->m_queues.resize(queueFamilyCount);
    for(auto queueFamilyIndex = 0U; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
    {
        device->m_queues[queueFamilyIndex].resize(queueCreateInfos[queueFamilyIndex].queueCount);
        for(auto queueIndex = 0U; queueIndex < queueCreateInfos[queueFamilyIndex].queueCount; ++queueIndex)
        {
            VkQueue queue = VK_NULL_HANDLE;
            vkGetDeviceQueue(handle, queueFamilyIndex, queueIndex, &queue);
            device->m_queues[queueFamilyIndex][queueIndex] =
                new VulkanQueue(device, queue, queueFamilyIndex, queueIndex, queueFamilyProperties[queueFamilyIndex]);
        }
    }

    // Copy address of object instance.
    *ppDevice = device;

    // Return success.
    return VK_SUCCESS;
}

void VulkanDevice::Destroy(VulkanDevice *pDevice)
{
    for(auto &[_, commandpool] : pDevice->m_commandPools)
    {
        pDevice->destroyCommandPool(commandpool);
    }

    if(pDevice->m_handle)
    {
        vkDestroyDevice(pDevice->m_handle, nullptr);
    }
    delete pDevice;
}

VkResult VulkanDevice::createCommandPool(const CommandPoolCreateInfo& createInfo, VulkanCommandPool **ppPool)
{
    VkCommandPoolCreateInfo cmdPoolInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = createInfo.flags,
        .queueFamilyIndex = createInfo.queueFamilyIndex,
    };

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateCommandPool(m_handle, &cmdPoolInfo, nullptr, &cmdPool));
    *ppPool = new VulkanCommandPool(createInfo, this, cmdPool);
    return VK_SUCCESS;
}

VkFormat VulkanDevice::getDepthFormat() const
{
    return m_physicalDevice->findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkResult VulkanDevice::createImageView(const ImageViewCreateInfo &createInfo, VulkanImageView **ppImageView,
                                       VulkanImage *pImage)
{
    // Create a new Vulkan image view.
    VkImageViewCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = pImage->getHandle(),
        .viewType = static_cast<VkImageViewType>(createInfo.viewType),
        .format = static_cast<VkFormat>(createInfo.format),
    };
    info.subresourceRange = {
        .aspectMask = aph::utils::getImageAspectFlags(static_cast<VkFormat>(createInfo.format)),
        .baseMipLevel = createInfo.subresourceRange.baseMipLevel,
        .levelCount = createInfo.subresourceRange.levelCount,
        .baseArrayLayer = createInfo.subresourceRange.baseArrayLayer,
        .layerCount = createInfo.subresourceRange.layerCount,
    };
    memcpy(&info.components, &createInfo.components, sizeof(VkComponentMapping));

    VkImageView handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateImageView(getHandle(), &info, nullptr, &handle));

    *ppImageView = new VulkanImageView(createInfo, pImage, handle);

    return VK_SUCCESS;
}

VkResult VulkanDevice::executeSingleCommands(QueueTypeFlags type, const std::function<void(VulkanCommandBuffer *pCmdBuffer)> &&func)
{
    VulkanCommandBuffer *cmd = nullptr;
    VK_CHECK_RESULT(allocateCommandBuffers(1, &cmd, getQueueByFlags(type)));

    VK_CHECK_RESULT(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
    func(cmd);
    VK_CHECK_RESULT(cmd->end());

    uint32_t queueFamilyIndex = getQueueByFlags(type)->getFamilyIndex();
    auto *queue = m_queues[queueFamilyIndex][0];

    QueueSubmitInfo submitInfo{.commandBuffers = {cmd}};
    VK_CHECK_RESULT(queue->submit({submitInfo}, VK_NULL_HANDLE));
    VK_CHECK_RESULT(queue->waitIdle());

    freeCommandBuffers(1, &cmd);

    return VK_SUCCESS;
}

VkResult VulkanDevice::createBuffer(const BufferCreateInfo &createInfo, VulkanBuffer **ppBuffer, const void *data)
{
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = createInfo.size,
        .usage = createInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer buffer;
    VK_CHECK_RESULT(vkCreateBuffer(getHandle(), &bufferInfo, nullptr, &buffer));

    // create memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_handle, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo = aph::init::memoryAllocateInfo(
        memRequirements.size, m_physicalDevice->findMemoryType(memRequirements.memoryTypeBits, createInfo.property));
    VkDeviceMemory memory;
    VK_CHECK_RESULT(vkAllocateMemory(m_handle, &allocInfo, nullptr, &memory));

    *ppBuffer = new VulkanBuffer(this, createInfo, buffer, memory);

    // bind buffer and memory
    VK_CHECK_RESULT((*ppBuffer)->bind());

    if(data)
    {
        (*ppBuffer)->map();
        (*ppBuffer)->copyTo(data, (*ppBuffer)->getSize());
        (*ppBuffer)->unmap();
    }

    return VK_SUCCESS;
}

VkResult VulkanDevice::createImage(const ImageCreateInfo &createInfo, VulkanImage **ppImage)
{
    VkImage image;
    VkDeviceMemory memory;

    VkImageCreateInfo imageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = createInfo.flags,
        .imageType = static_cast<VkImageType>(createInfo.imageType),
        .format = static_cast<VkFormat>(createInfo.format),
        .mipLevels = createInfo.mipLevels,
        .arrayLayers = createInfo.layerCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = static_cast<VkImageTiling>(createInfo.tiling),
        .usage = createInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    imageCreateInfo.extent.width = createInfo.extent.width;
    imageCreateInfo.extent.height = createInfo.extent.height;
    imageCreateInfo.extent.depth = createInfo.extent.depth;

    VK_CHECK_RESULT(vkCreateImage(m_handle, &imageCreateInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_handle, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = m_physicalDevice->findMemoryType(memRequirements.memoryTypeBits, createInfo.property),
    };

    VK_CHECK_RESULT(vkAllocateMemory(m_handle, &allocInfo, nullptr, &memory));

    *ppImage = new VulkanImage(this, createInfo, image, memory);

    if((*ppImage)->getMemory() != VK_NULL_HANDLE)
    {
        auto result = (*ppImage)->bind();
        return result;
    }

    return VK_SUCCESS;
}

VulkanPhysicalDevice *VulkanDevice::getPhysicalDevice() const
{
    return m_physicalDevice;
}

VkResult VulkanDevice::createFramebuffers(const FramebufferCreateInfo &createInfo, VulkanFramebuffer **ppFramebuffer)
{
    return VulkanFramebuffer::Create(this, createInfo, ppFramebuffer);
}

void VulkanDevice::destroyBuffer(VulkanBuffer *pBuffer)
{
    if(pBuffer->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_handle, pBuffer->getMemory(), nullptr);
    }
    vkDestroyBuffer(m_handle, pBuffer->getHandle(), nullptr);
    delete pBuffer;
}
void VulkanDevice::destroyImage(VulkanImage *pImage)
{
    if(pImage->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_handle, pImage->getMemory(), nullptr);
    }
    vkDestroyImage(m_handle, pImage->getHandle(), nullptr);
    delete pImage;
    pImage = nullptr;
}
void VulkanDevice::destroyImageView(VulkanImageView *pImageView)
{
    vkDestroyImageView(m_handle, pImageView->getHandle(), nullptr);
    delete pImageView;
    pImageView = nullptr;
}
void VulkanDevice::destoryRenderPass(VulkanRenderPass *pRenderpass)
{
    vkDestroyRenderPass(m_handle, pRenderpass->getHandle(), nullptr);
    delete pRenderpass;
}
void VulkanDevice::destroyFramebuffers(VulkanFramebuffer *pFramebuffer)
{
    delete pFramebuffer;
}
VkResult VulkanDevice::createSwapchain(const SwapChainCreateInfo& createInfo, VulkanSwapChain **ppSwapchain)
{
    *ppSwapchain = new VulkanSwapChain(createInfo, this);
    return VK_SUCCESS;
}

void VulkanDevice::destroySwapchain(VulkanSwapChain *pSwapchain)
{
    vkDestroySwapchainKHR(getHandle(), pSwapchain->getHandle(), nullptr);
    delete pSwapchain;
}

VulkanQueue *VulkanDevice::getQueueByFlags(QueueTypeFlags flags, uint32_t queueIndex)
{
    std::vector<uint32_t> supportedQueueFamilyIndexList = m_physicalDevice->getQueueFamilyIndexByFlags(flags);
    if (supportedQueueFamilyIndexList.empty())
    {
        return nullptr;
    }
    return m_queues[supportedQueueFamilyIndexList[0]][queueIndex];
}

VkResult VulkanDevice::waitIdle()
{
    return vkDeviceWaitIdle(getHandle());
}

VulkanCommandPool *VulkanDevice::getCommandPoolWithQueue(VulkanQueue *queue)
{
    auto queueIndices = queue->getFamilyIndex();

    if(m_commandPools.count(queueIndices))
    {
        return m_commandPools.at(queueIndices);
    }

    CommandPoolCreateInfo createInfo{.queueFamilyIndex = queueIndices};
    VulkanCommandPool *pool = nullptr;
    createCommandPool(createInfo, &pool);
    m_commandPools[queueIndices] = pool;
    return pool;
}

void VulkanDevice::destroyCommandPool(VulkanCommandPool *pPool)
{
    vkDestroyCommandPool(getHandle(), pPool->getHandle(), nullptr);
    delete pPool;
}

VkResult VulkanDevice::allocateCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers,
                                              VulkanQueue * pQueue)
{
    auto *queue = pQueue;
    auto *pool = getCommandPoolWithQueue(queue);

    std::vector<VkCommandBuffer> handles(commandBufferCount);
    VK_CHECK_RESULT(pool->allocateCommandBuffers(commandBufferCount, handles.data()));

    for(auto i = 0; i < commandBufferCount; i++)
    {
        ppCommandBuffers[i] = new VulkanCommandBuffer(pool, handles[i], pool->getQueueFamilyIndex());
    }
    return VK_SUCCESS;
}

void VulkanDevice::freeCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers)
{
    // Destroy all of the command buffers.
    for(auto i = 0U; i < commandBufferCount; ++i)
    {
        delete ppCommandBuffers[i];
    }
}
VkResult VulkanDevice::createGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo,
                                              VulkanRenderPass *pRenderPass, VulkanPipeline **ppPipeline)
{
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &createInfo.viewport,
        .scissorCount = 1,
        .pScissors = &createInfo.scissor,
    };

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &createInfo.colorBlendAttachment,
    };

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    {
        std::vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(createInfo.setLayouts.size());
        for(auto *setLayout : createInfo.setLayouts)
        {
            setLayouts.push_back(setLayout->getHandle());
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            aph::init::pipelineLayoutCreateInfo(setLayouts, createInfo.constants);
        vkCreatePipelineLayout(getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
    }

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pVertexInputState = &createInfo.vertexInputInfo,
        .pInputAssemblyState = &createInfo.inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &createInfo.rasterizer,
        .pMultisampleState = &createInfo.multisampling,
        .pDepthStencilState = &createInfo.depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &createInfo.dynamicState,
        .layout = pipelineLayout,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    if(pRenderPass){
        pipelineInfo.renderPass = pRenderPass->getHandle();
    }
    else{
        pipelineInfo.pNext = &createInfo.renderingCreateInfo;
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for(const auto &[stage, sModule] : createInfo.shaderMapList)
    {
        shaderStages.push_back(aph::init::pipelineShaderStageCreateInfo(stage, sModule->getHandle()));
    }
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();

    VkPipeline handle;
    VK_CHECK_RESULT(
        vkCreateGraphicsPipelines(getHandle(), createInfo.pipelineCache, 1, &pipelineInfo, nullptr, &handle));

    *ppPipeline = VulkanPipeline::CreateGraphicsPipeline(this, createInfo, pRenderPass, pipelineLayout, handle);

    return VK_SUCCESS;
}

void VulkanDevice::destroyPipeline(VulkanPipeline *pipeline)
{
    vkDestroyPipelineLayout(getHandle(), pipeline->getPipelineLayout(), nullptr);
    vkDestroyPipeline(getHandle(), pipeline->getHandle(), nullptr);
    delete pipeline;
}

VkResult VulkanDevice::createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo &createInfo,
                                                 VulkanDescriptorSetLayout **ppDescriptorSetLayout)
{
    VkDescriptorSetLayout setLayout;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_handle, &createInfo, nullptr, &setLayout));
    *ppDescriptorSetLayout = new VulkanDescriptorSetLayout(this, createInfo, setLayout);
    return VK_SUCCESS;
}

void VulkanDevice::destroyDescriptorSetLayout(VulkanDescriptorSetLayout *pLayout)
{
    vkDestroyDescriptorSetLayout(m_handle, pLayout->getHandle(), nullptr);
    delete pLayout;
}
VkResult VulkanDevice::createComputePipeline(const ComputePipelineCreateInfo &createInfo, VulkanPipeline **ppPipeline)
{
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    {
        std::vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(createInfo.setLayouts.size());
        for(auto *setLayout : createInfo.setLayouts)
        {
            setLayouts.push_back(setLayout->getHandle());
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            aph::init::pipelineLayoutCreateInfo(setLayouts, createInfo.constants);
        VK_CHECK_RESULT(vkCreatePipelineLayout(getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout));
    }
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
    for(const auto &[stage, sModule] : createInfo.shaderMapList)
    {
        shaderStages.push_back(aph::init::pipelineShaderStageCreateInfo(stage, sModule->getHandle()));
    }

    VkComputePipelineCreateInfo ci = aph::init::computePipelineCreateInfo(pipelineLayout);
    ci.stage = shaderStages[0];
    VkPipeline handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateComputePipelines(this->getHandle(), VK_NULL_HANDLE, 1, &ci, nullptr, &handle));
    *ppPipeline = VulkanPipeline::CreateComputePipeline(this, createInfo, pipelineLayout, handle);
    return VK_SUCCESS;
}

VkResult VulkanDevice::createRenderPass(const RenderPassCreateInfo &createInfo, VulkanRenderPass **ppRenderPass)
{
    std::vector<VkAttachmentDescription> attachments {};
    std::vector<VkAttachmentReference> colorAttachmentRefs {};
    VkAttachmentReference depthAttachmentRef {};

    const auto &colorAttachments = createInfo.colorAttachments;
    for(uint32_t idx = 0; idx < colorAttachments.size(); idx++)
    {
        attachments.push_back(colorAttachments[idx]);
        VkAttachmentReference ref{
            .attachment = idx,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        colorAttachmentRefs.push_back(ref);
    }

    VkSubpassDescription subpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size()),
        .pColorAttachments = colorAttachmentRefs.data(),
    };

    if (createInfo.depthAttachment.has_value())
    {
        depthAttachmentRef = {
            .attachment = static_cast<uint32_t>(colorAttachments.size()),
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        attachments.push_back(createInfo.depthAttachment.value());
        subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;
    }


    std::array<VkSubpassDependency, 1> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_NONE_KHR;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = dependencies.size(),
        .pDependencies = dependencies.data(),
    };

    VkRenderPass renderpass;
    VK_CHECK_RESULT(vkCreateRenderPass(m_handle, &renderPassInfo, nullptr, &renderpass));

    *ppRenderPass = new VulkanRenderPass(renderpass, colorAttachmentRefs.size());

    return VK_SUCCESS;
}
VkResult VulkanDevice::waitForFence(const std::vector<VkFence> &fences, bool waitAll, uint32_t timeout)
{
    return vkWaitForFences(getHandle(), fences.size(), fences.data(), VK_TRUE, UINT64_MAX);
}
}  // namespace aph
