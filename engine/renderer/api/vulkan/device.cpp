#include "device.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "descriptorSetLayout.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "queue.h"
#include "renderpass.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitivesPool.h"
#include "vkInit.hpp"
#include "vkUtils.h"
#include "vulkan/vulkan_core.h"

namespace vkl
{

#ifdef VK_CHECK_RESULT
#undef VK_CHECK_RESULT
#endif

#define VK_CHECK_RESULT(f) \
    { \
        VkResult res = (f); \
        if(res != VK_SUCCESS) \
        { \
            return res; \
        } \
    }


VkResult VulkanDevice::Create(VulkanPhysicalDevice *pPhysicalDevice, const DeviceCreateInfo *pCreateInfo,
                              VulkanDevice **ppDevice)
{
    auto &queueFamilyProperties = pPhysicalDevice->getQueueFamilyProperties();
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
    VkPhysicalDeviceFeatures enabledFeatures = {};
    vkGetPhysicalDeviceFeatures(pPhysicalDevice->getHandle(), &enabledFeatures);

    // Create the Vulkan device.
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = pCreateInfo->pNext,
        .queueCreateInfoCount = (uint32_t)queueCreateInfos.size(),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = pCreateInfo->enabledLayerCount,
        .ppEnabledLayerNames = pCreateInfo->ppEnabledLayerNames,
        .enabledExtensionCount = pCreateInfo->enabledExtensionCount,
        .ppEnabledExtensionNames = pCreateInfo->ppEnabledExtensionNames,
        .pEnabledFeatures = &pPhysicalDevice->getDeviceFeatures(),
    };

    VkDevice handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateDevice(pPhysicalDevice->getHandle(), &deviceCreateInfo, nullptr, &handle));

    // Initialize Device class.
    auto *device = new VulkanDevice;
    memcpy(&device->_createInfo, pCreateInfo, sizeof(DeviceCreateInfo));
    device->_handle = handle;
    device->_physicalDevice = pPhysicalDevice;
    device->_syncPrimitivesPool = new VulkanSyncPrimitivesPool(device);
    device->_shaderCache = new VulkanShaderCache(device);

    // Get handles to all of the previously enumerated and created queues.
    device->_queues.resize(queueFamilyCount);
    for(auto queueFamilyIndex = 0U; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
    {
        for(auto queueIndex = 0U; queueIndex < queueCreateInfos[queueFamilyIndex].queueCount;
            ++queueIndex)
        {
            VkQueue queue = VK_NULL_HANDLE;
            vkGetDeviceQueue(handle, queueFamilyIndex, queueIndex, &queue);
            if(queue)
            {
                device->_queues[queueFamilyIndex].push_back(
                    new VulkanQueue(device, queue, queueFamilyIndex, queueIndex,
                                    queueFamilyProperties[queueFamilyIndex]));
            }
        }
    }

    // Copy address of object instance.
    *ppDevice = device;

    // Return success.
    return VK_SUCCESS;
}

void VulkanDevice::Destroy(VulkanDevice *pDevice)
{
    for(auto &[_, commandpool] : pDevice->_commandPools)
    {
        pDevice->destroyCommandPool(commandpool);
    }

    if(pDevice->_shaderCache)
    {
        pDevice->_shaderCache->destroy();
    }

    if(pDevice->_syncPrimitivesPool)
    {
        delete pDevice->_syncPrimitivesPool;
    }

    if(pDevice->_handle)
    {
        vkDestroyDevice(pDevice->_handle, nullptr);
    }
    delete pDevice;
}

VkResult VulkanDevice::createCommandPool(VulkanCommandPool **ppPool, uint32_t queueFamilyIndex,
                                         VkCommandPoolCreateFlags createFlags)
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = createFlags;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateCommandPool(_handle, &cmdPoolInfo, nullptr, &cmdPool));

    *ppPool = VulkanCommandPool::Create(this, queueFamilyIndex, cmdPool);
    return VK_SUCCESS;
}

VkFormat VulkanDevice::getDepthFormat() const
{
    return _physicalDevice->findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkResult VulkanDevice::createImageView(const ImageViewCreateInfo &createInfo, VulkanImageView **ppImageView,
                                       VulkanImage *pImage)
{
    // Create a new Vulkan image view.
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.image = pImage->getHandle();
    info.viewType = static_cast<VkImageViewType>(createInfo.viewType);
    info.format = static_cast<VkFormat>(createInfo.format);
    memcpy(&info.components, &createInfo.components, sizeof(VkComponentMapping));
    info.subresourceRange.aspectMask =
        vkl::utils::getImageAspectFlags(static_cast<VkFormat>(createInfo.format));
    info.subresourceRange.baseMipLevel = createInfo.subresourceRange.baseMipLevel;
    info.subresourceRange.levelCount = createInfo.subresourceRange.levelCount;
    info.subresourceRange.baseArrayLayer = createInfo.subresourceRange.baseArrayLayer;
    info.subresourceRange.layerCount = createInfo.subresourceRange.layerCount;
    VkImageView handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateImageView(pImage->getDevice()->getHandle(), &info, nullptr, &handle));

    // [TODO]
    auto ci = createInfo;
    *ppImageView = VulkanImageView::createFromHandle(&ci, pImage, handle);

    return VK_SUCCESS;
}

void VulkanDevice::endSingleTimeCommands(VulkanCommandBuffer *commandBuffer)
{
    uint32_t queueFamilyIndex = commandBuffer->getQueueFamilyIndices();
    auto queue = _queues[queueFamilyIndex][0];

    commandBuffer->end();

    VkSubmitInfo submitInfo = vkl::init::submitInfo(&commandBuffer->getHandle());
    queue->submit(1, &submitInfo, VK_NULL_HANDLE);
    queue->waitIdle();

    freeCommandBuffers(1, &commandBuffer);
}

VulkanCommandBuffer *VulkanDevice::beginSingleTimeCommands(VkQueueFlags flags)
{
    VulkanCommandBuffer *instance = nullptr;
    allocateCommandBuffers(1, &instance, flags);
    instance->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return instance;
}

VkResult VulkanDevice::createBuffer(const BufferCreateInfo &createInfo, VulkanBuffer **ppBuffer, void *data)
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = createInfo.size,
        .usage = createInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK_RESULT(vkCreateBuffer(_handle, &bufferInfo, nullptr, &buffer));

    // create memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_handle, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo = vkl::init::memoryAllocateInfo(
        memRequirements.size,
        _physicalDevice->findMemoryType(memRequirements.memoryTypeBits, createInfo.property));
    VK_CHECK_RESULT(vkAllocateMemory(_handle, &allocInfo, nullptr, &memory));

    // [TODO]
    auto ci = createInfo;
    *ppBuffer = VulkanBuffer::CreateFromHandle(this, &ci, buffer, memory);

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
        .imageType = VK_IMAGE_TYPE_2D,
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

    VK_CHECK_RESULT(vkCreateImage(_handle, &imageCreateInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_handle, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            _physicalDevice->findMemoryType(memRequirements.memoryTypeBits, createInfo.property),
    };

    VK_CHECK_RESULT(vkAllocateMemory(_handle, &allocInfo, nullptr, &memory));

    auto ci = createInfo;
    *ppImage = VulkanImage::CreateFromHandle(this, &ci, image, memory);

    if((*ppImage)->getMemory() != VK_NULL_HANDLE)
    {
        auto result = (*ppImage)->bind();
        return result;
    }

    return VK_SUCCESS;
}

VulkanPhysicalDevice *VulkanDevice::getPhysicalDevice() const
{
    return _physicalDevice;
}

VkResult VulkanDevice::createRenderPass(RenderPassCreateInfo *createInfo,
                                        VulkanRenderPass **ppRenderPass,
                                        const std::vector<VkAttachmentDescription> &colorAttachments,
                                        const VkAttachmentDescription &depthAttachment)
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;

    for(uint32_t idx = 0; idx < colorAttachments.size(); idx++)
    {
        attachments.push_back(colorAttachments[idx]);
        VkAttachmentReference ref{};
        ref.attachment = idx;
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(ref);
    }

    attachments.push_back(depthAttachment);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = static_cast<uint32_t>(colorAttachments.size());
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpassDescription.pColorAttachments = colorAttachmentRefs.data();
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

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
    VK_CHECK_RESULT(vkCreateRenderPass(_handle, &renderPassInfo, nullptr, &renderpass));

    *ppRenderPass = new VulkanRenderPass(renderpass, colorAttachmentRefs.size());

    return VK_SUCCESS;
}

VkResult VulkanDevice::createFramebuffers(FramebufferCreateInfo *pCreateInfo,
                                          VulkanFramebuffer **ppFramebuffer, uint32_t attachmentCount,
                                          VulkanImageView **pAttachments)
{
    return VulkanFramebuffer::Create(this, pCreateInfo, ppFramebuffer, attachmentCount, pAttachments);
}

void VulkanDevice::destroyBuffer(VulkanBuffer *pBuffer)
{
    if(pBuffer->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(_handle, pBuffer->getMemory(), nullptr);
    }
    vkDestroyBuffer(_handle, pBuffer->getHandle(), nullptr);
    delete pBuffer;
}
void VulkanDevice::destroyImage(VulkanImage *pImage)
{
    if(pImage->getMemory() != VK_NULL_HANDLE)
    {
        vkFreeMemory(_handle, pImage->getMemory(), nullptr);
    }
    vkDestroyImage(_handle, pImage->getHandle(), nullptr);
    delete pImage;
    pImage = nullptr;
}
void VulkanDevice::destroyImageView(VulkanImageView *pImageView)
{
    vkDestroyImageView(_handle, pImageView->getHandle(), nullptr);
    delete pImageView;
    pImageView = nullptr;
}
void VulkanDevice::destoryRenderPass(VulkanRenderPass *pRenderpass)
{
    vkDestroyRenderPass(_handle, pRenderpass->getHandle(), nullptr);
    delete pRenderpass;
}
void VulkanDevice::destroyFramebuffers(VulkanFramebuffer *pFramebuffer)
{
    delete pFramebuffer;
}
VkResult VulkanDevice::createSwapchain(VkSurfaceKHR surface, VulkanSwapChain **ppSwapchain,
                                       WindowData *data)
{
    VulkanSwapChain *instance = VulkanSwapChain::Create(this, surface, data);

    *ppSwapchain = instance;

    return VK_SUCCESS;
}

void VulkanDevice::destroySwapchain(VulkanSwapChain *pSwapchain)
{
    vkDestroySwapchainKHR(getHandle(), pSwapchain->getHandle(), nullptr);
    delete pSwapchain;
}

VulkanQueue *VulkanDevice::getQueueByFlags(VkQueueFlags flags, uint32_t queueIndex)
{
    const std::vector<VkQueueFamilyProperties> &queueFamilyProperties =
        _physicalDevice->getQueueFamilyProperties();

    // Iterate over queues in order to find one matching requested flags.
    // Favor queue families matching only what's specified in queueFlags over
    // families having other bits set as well.
    VkQueueFlags minFlags = ~0;
    VulkanQueue *bestQueue = nullptr;
    for(auto queueFamilyIndex = 0U; queueFamilyIndex < queueFamilyProperties.size(); ++queueFamilyIndex)
    {
        if(((queueFamilyProperties[queueFamilyIndex].queueFlags & flags) == flags) &&
           queueIndex < queueFamilyProperties[queueFamilyIndex].queueCount)
        {
            if(queueFamilyProperties[queueFamilyIndex].queueFlags < minFlags)
            {
                minFlags = queueFamilyProperties[queueFamilyIndex].queueFlags;
                bestQueue = _queues[queueFamilyIndex][queueIndex];
            }
        }
    }

    // Return the queue for the given flags.
    return bestQueue;
}
void VulkanDevice::waitIdle()
{
    vkDeviceWaitIdle(getHandle());
}

VulkanCommandPool *VulkanDevice::getCommandPoolWithQueue(VulkanQueue *queue)
{
    auto indices = queue->getFamilyIndex();

    if(_commandPools.count(indices))
    {
        return _commandPools.at(indices);
    }

    VulkanCommandPool *pool = nullptr;
    createCommandPool(&pool, indices);
    _commandPools[indices] = pool;
    return pool;
}

void VulkanDevice::destroyCommandPool(VulkanCommandPool *pPool)
{
    vkDestroyCommandPool(getHandle(), pPool->getHandle(), nullptr);
    delete pPool;
}

VkResult VulkanDevice::allocateCommandBuffers(uint32_t commandBufferCount,
                                              VulkanCommandBuffer **ppCommandBuffers, VkQueueFlags flags)
{
    auto *queue = getQueueByFlags(flags);
    auto *pool = getCommandPoolWithQueue(queue);

    std::vector<VkCommandBuffer> handles(commandBufferCount);
    VK_CHECK_RESULT(pool->allocateCommandBuffers(commandBufferCount, handles.data()));

    for(auto i = 0; i < commandBufferCount; i++)
    {
        ppCommandBuffers[i] = new VulkanCommandBuffer(pool, handles[i], pool->getQueueFamilyIndex());
    }
    return VK_SUCCESS;
}

void VulkanDevice::freeCommandBuffers(uint32_t commandBufferCount,
                                      VulkanCommandBuffer **ppCommandBuffers)
{
    // Destroy all of the command buffers.
    for(auto i = 0U; i < commandBufferCount; ++i)
    {
        delete ppCommandBuffers[i];
    }
}
VkResult VulkanDevice::createGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo,
                                              VulkanRenderPass *pRenderPass,
                                              VulkanPipeline **ppPipeline)
{
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &createInfo.viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &createInfo.scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &createInfo.colorBlendAttachment;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    {
        std::vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(createInfo.setLayouts.size());
        for(auto setLayout : createInfo.setLayouts)
        {
            setLayouts.push_back(setLayout->getHandle());
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, createInfo.constants);
        vkCreatePipelineLayout(getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
    }

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for(const auto &[stage, sModule] : createInfo.shaderMapList)
    {
        shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage, sModule->getHandle()));
    }
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &createInfo.vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &createInfo.inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = &createInfo.dynamicState;
    pipelineInfo.pRasterizationState = &createInfo.rasterizer;
    pipelineInfo.pDepthStencilState = &createInfo.depthStencil;
    pipelineInfo.pMultisampleState = &createInfo.multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = pRenderPass->getHandle();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline handle;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(getHandle(), createInfo.pipelineCache, 1, &pipelineInfo,
                                            nullptr, &handle));

    *ppPipeline = VulkanPipeline::CreateGraphicsPipeline(this, createInfo, pRenderPass, pipelineLayout, handle);

    return VK_SUCCESS;
}

void VulkanDevice::destroyPipeline(VulkanPipeline *pipeline)
{
    vkDestroyPipelineLayout(getHandle(), pipeline->getPipelineLayout(), nullptr);
    vkDestroyPipeline(getHandle(), pipeline->getHandle(), nullptr);
    delete pipeline;
}

VkResult VulkanDevice::createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                 VulkanDescriptorSetLayout **ppDescriptorSetLayout)
{
    VkDescriptorSetLayout setLayout;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_handle, pCreateInfo, nullptr, &setLayout));
    *ppDescriptorSetLayout = VulkanDescriptorSetLayout::Create(this, pCreateInfo, setLayout);
    return VK_SUCCESS;
}

void VulkanDevice::destroyDescriptorSetLayout(VulkanDescriptorSetLayout *pLayout)
{
    vkDestroyDescriptorSetLayout(_handle, pLayout->getHandle(), nullptr);
    delete pLayout;
}
VkResult VulkanDevice::createComputePipeline(const ComputePipelineCreateInfo& createInfo, VulkanPipeline **ppPipeline)
{
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    {
        std::vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(createInfo.setLayouts.size());
        for(auto setLayout : createInfo.setLayouts)
        {
            setLayouts.push_back(setLayout->getHandle());
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, createInfo.constants);
        VK_CHECK_RESULT(vkCreatePipelineLayout(getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout));
    }
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
    for(const auto &[stage, sModule] : createInfo.shaderMapList)
    {
        shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage, sModule->getHandle()));
    }
    VkComputePipelineCreateInfo ci = vkl::init::computePipelineCreateInfo(pipelineLayout);
    ci.stage = shaderStages[0];
    VkPipeline handle = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateComputePipelines(this->getHandle(), VK_NULL_HANDLE, 1, &ci, nullptr, &handle));
    *ppPipeline = VulkanPipeline::CreateComputePipeline(this, createInfo, pipelineLayout, handle);
    return VK_SUCCESS;
}

VkResult VulkanDevice::createRenderPass(RenderPassCreateInfo *createInfo,
                                        VulkanRenderPass **ppRenderPass,
                                        const std::vector<VkAttachmentDescription> &colorAttachments)
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;

    for(uint32_t idx = 0; idx < colorAttachments.size(); idx++)
    {
        attachments.push_back(colorAttachments[idx]);
        VkAttachmentReference ref{};
        ref.attachment = idx;
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(ref);
    }

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpassDescription.pColorAttachments = colorAttachmentRefs.data();
    subpassDescription.pDepthStencilAttachment = nullptr;

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
    VK_CHECK_RESULT(vkCreateRenderPass(_handle, &renderPassInfo, nullptr, &renderpass));

    *ppRenderPass = new VulkanRenderPass(renderpass, colorAttachmentRefs.size());

    return VK_SUCCESS;
}
VkResult VulkanDevice::createRenderPass(RenderPassCreateInfo *createInfo,
                                        VulkanRenderPass **ppRenderPass,
                                        const VkAttachmentDescription &depthAttachment)
{
    std::vector<VkAttachmentDescription> attachments;

    attachments.push_back(depthAttachment);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 0;
    subpassDescription.pColorAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

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
    VK_CHECK_RESULT(vkCreateRenderPass(_handle, &renderPassInfo, nullptr, &renderpass));

    *ppRenderPass = new VulkanRenderPass(renderpass, 0);

    return VK_SUCCESS;
}
}  // namespace vkl
