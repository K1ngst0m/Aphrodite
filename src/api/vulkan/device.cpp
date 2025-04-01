#include "device.h"
#include "bindless.h"
#include "renderdoc_app.h"
#include "vmaAllocator.h"

#include "common/profiler.h"

#include "module/module.h"
#include "resource/shaderReflector.h"

const VkAllocationCallbacks* gVkAllocator = aph::vk::vkAllocator();

namespace aph::vk
{
Device::Device(const CreateInfoType& createInfo, PhysicalDevice* pPhysicalDevice, HandleType handle)
    : ResourceHandle(handle, createInfo)
    , m_resourcePool(this)
{
}

std::unique_ptr<Device> Device::Create(const DeviceCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();
    PhysicalDevice* gpu = createInfo.pPhysicalDevice;

    const auto& queueFamilyProperties = gpu->getHandle().getQueueFamilyProperties();
    const auto queueFamilyCount = queueFamilyProperties.size();

    // Allocate handles for all available queues.
    SmallVector<::vk::DeviceQueueCreateInfo> queueCreateInfos(queueFamilyCount);
    SmallVector<SmallVector<float>> priorities(queueFamilyCount);
    for (auto i = 0U; i < queueFamilyCount; ++i)
    {
        const float defaultPriority = 1.0f;
        priorities[i].resize(queueFamilyProperties[i].queueCount, defaultPriority);
        queueCreateInfos[i]
            .setQueueFamilyIndex(i)
            .setQueueCount(queueFamilyProperties[i].queueCount)
            .setPQueuePriorities(priorities[i].data());
    }

    SmallVector<const char*> requiredExtensions;
    {
        const auto& feature = createInfo.enabledFeatures;

        // Setup required extensions based on feature requirements
        gpu->setupRequiredExtensions(feature, requiredExtensions);

        // adding addition extension/features
        {
        }

        // Enable required features
        gpu->enableFeatures(feature);
    }

    // verify extension support
    {
        bool allExtensionSupported = true;
        for (const auto& requiredExtension : requiredExtensions)
        {
            if (!gpu->checkExtensionSupported(requiredExtension))
            {
                VK_LOG_ERR("The device extension %s is not supported.", requiredExtension);
                allExtensionSupported = false;
            }
        }
        if (!allExtensionSupported)
        {
            APH_ASSERT(false);
            return nullptr;
        }
    }

    // Validate features against hardware support
    if (!gpu->validateFeatures(createInfo.enabledFeatures))
    {
        VK_LOG_ERR("Critical GPU features not supported by hardware");
        return nullptr;
    }

    // Enable all physical device available features.
    ::vk::PhysicalDeviceFeatures supportedFeatures = gpu->getHandle().getFeatures();
    ::vk::PhysicalDeviceFeatures2 supportedFeatures2 = gpu->getHandle().getFeatures();

    // TODO
    supportedFeatures.sampleRateShading = VK_TRUE;
    supportedFeatures.samplerAnisotropy = VK_TRUE;

    supportedFeatures2.setPNext(gpu->getRequestedFeatures()).setFeatures(supportedFeatures);

    ::vk::DeviceCreateInfo device_create_info{};
    device_create_info.setPNext(&supportedFeatures2)
        .setQueueCreateInfos(queueCreateInfos)
        .setPEnabledExtensionNames(requiredExtensions);

    auto [result, device_handle] = gpu->getHandle().createDevice(device_create_info, vk_allocator());
    VK_VR(result);

    // Initialize Device class.
    auto device = std::unique_ptr<Device>(new Device(createInfo, gpu, device_handle));

    // TODO
    device->m_resourcePool.deviceMemory = std::make_unique<VMADeviceAllocator>(createInfo.pInstance, device.get());
    device->m_resourcePool.bindless = std::make_unique<BindlessResource>(device.get());

    {
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
        {
            auto& queueFamily = queueFamilyProperties[queueFamilyIndex];
            auto queueFlags = queueFamily.queueFlags;

            QueueType queueType = QueueType::Unsupport;
            // universal queue
            if (queueFlags & ::vk::QueueFlagBits::eGraphics)
            {
                VK_LOG_DEBUG("create graphics queue %lu", queueFamilyIndex);
                queueType = QueueType::Graphics;
            }
            // compute queue
            else if (queueFlags & ::vk::QueueFlagBits::eCompute)
            {
                VK_LOG_DEBUG("Found compute queue %lu", queueFamilyIndex);
                queueType = QueueType::Compute;
            }
            // transfer queue
            else if (queueFlags & ::vk::QueueFlagBits::eTransfer)
            {
                VK_LOG_DEBUG("Found transfer queue %lu", queueFamilyIndex);
                queueType = QueueType::Transfer;
            }

            for (auto queueIndex = 0U; queueIndex < queueCreateInfos[queueFamilyIndex].queueCount; ++queueIndex)
            {
                ::vk::DeviceQueueInfo2 queueInfo{};
                queueInfo.setQueueFamilyIndex(queueFamilyIndex).setQueueIndex(queueIndex);
                ::vk::Queue queue = device_handle.getQueue2(queueInfo);
                device->m_queues[queueType].push_back(
                    device->m_resourcePool.queue.allocate(queue, queueFamilyIndex, queueIndex, queueType));
            }
        }
    }

    if (createInfo.enabledFeatures.capture)
    {
        if (auto res = device->initCapture(); res.success())
        {
            VK_LOG_INFO("Renderdoc plugin loaded.");
        }
        else
        {
            VK_LOG_WARN("Failed to load renderdoc plugin: %s", res.toString());
        }
    }

    // Return success.
    return device;
}

void Device::Destroy(Device* pDevice)
{
    APH_PROFILER_SCOPE();
    APH_VR(pDevice->waitIdle());

    pDevice->m_resourcePool.bindless.reset();
    pDevice->m_resourcePool.program.clear();
    pDevice->m_resourcePool.syncPrimitive.clear();
    pDevice->m_resourcePool.setLayout.clear();
    pDevice->m_resourcePool.deviceMemory.reset();

    APH_ASSERT(pDevice->m_handle);
    pDevice->getHandle().destroy(vk_allocator());
}

Format Device::getDepthFormat() const
{
    APH_PROFILER_SCOPE();
    Format format = getPhysicalDevice()->findSupportedFormat({ Format::D32, Format::D32S8, Format::D24S8 },
                                                             ::vk::ImageTiling::eOptimal,
                                                             ::vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    return format;
}

Result Device::createImpl(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppLayout)
{
    APH_PROFILER_SCOPE();
    const SmallVector<::vk::DescriptorSetLayoutBinding>& vkBindings = createInfo.bindings;
    const SmallVector<::vk::DescriptorPoolSize>& poolSizes = createInfo.poolSizes;

    auto bindlessFlags =
        ::vk::DescriptorBindingFlagBits::eUpdateAfterBind | ::vk::DescriptorBindingFlagBits::ePartiallyBound;

    bool isBindless = false;
    for (const auto& vkBinding : vkBindings)
    {
        if (vkBinding.descriptorCount == VULKAN_NUM_BINDINGS_BINDLESS_VARYING)
        {
            isBindless = true;
        }
    }

    SmallVector<::vk::DescriptorBindingFlags> flags(vkBindings.size(), bindlessFlags);
    ::vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{};
    bindingFlagsCreateInfo.setBindingFlags(flags);

    ::vk::DescriptorSetLayoutCreateInfo vkCreateInfo = {};
    vkCreateInfo.setBindings(vkBindings);
    if (isBindless)
    {
        vkCreateInfo.setPNext(&bindingFlagsCreateInfo)
            .setFlags(::vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
    }

    auto [result, vkSetLayout] = getHandle().createDescriptorSetLayout(vkCreateInfo, vk_allocator());
    VK_VR(result);

    *ppLayout = m_resourcePool.setLayout.allocate(this, createInfo, vkSetLayout, poolSizes, vkBindings);
    return Result::Success;
}

Result Device::createImpl(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(createInfo.pPipelineLayout);
    bool hasTaskShader = false;
    SmallVector<Shader*> shaders{};

    // vs + fs
    if (createInfo.shaders.contains(ShaderStage::VS) && createInfo.shaders.contains(ShaderStage::FS))
    {
        shaders.push_back(createInfo.shaders.at(ShaderStage::VS));
        shaders.push_back(createInfo.shaders.at(ShaderStage::FS));
    }
    else if (createInfo.shaders.contains(ShaderStage::MS) && createInfo.shaders.contains(ShaderStage::FS))
    {
        if (createInfo.shaders.contains(ShaderStage::TS))
        {
            shaders.push_back(createInfo.shaders.at(ShaderStage::TS));
            hasTaskShader = true;
        }
        shaders.push_back(createInfo.shaders.at(ShaderStage::MS));
        shaders.push_back(createInfo.shaders.at(ShaderStage::FS));
    }
    // cs
    else if (createInfo.shaders.contains(ShaderStage::CS))
    {
        shaders.push_back(createInfo.shaders.at(ShaderStage::CS));
    }
    else
    {
        APH_ASSERT(false);
        return { Result::RuntimeError, "Unsupported shader stage combinations." };
    }

    SmallVector<::vk::DescriptorSetLayout> vkSetLayouts;
    for (auto setLayout : createInfo.pPipelineLayout->getSetLayouts())
    {
        vkSetLayouts.push_back(setLayout->getHandle());
    }

    HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps;
    // setup shader object
    {
        SmallVector<::vk::ShaderCreateInfoEXT> shaderCreateInfos;
        for (auto iter = shaders.cbegin(); iter != shaders.cend(); ++iter)
        {
            auto shader = *iter;
            ::vk::ShaderStageFlags nextStage = {};
            if (auto nextIter = std::next(iter); nextIter != shaders.cend())
            {
                nextStage = utils::VkCast((*nextIter)->getStage());
            }
            ::vk::ShaderCreateInfoEXT soCreateInfo{};
            soCreateInfo.setFlags(::vk::ShaderCreateFlagBitsEXT::eLinkStage)
                .setStage(utils::VkCast(shader->getStage()))
                .setNextStage(nextStage)
                .setPCode(shader->getCode().data())
                .setCodeSize(shader->getCode().size() * sizeof(shader->getCode()[0]))
                .setCodeType(::vk::ShaderCodeTypeEXT::eSpirv)
                .setPName(shader->getEntryPointName().data())
                .setSetLayouts(vkSetLayouts);

            if (!hasTaskShader && soCreateInfo.stage == ::vk::ShaderStageFlagBits::eMeshEXT)
            {
                soCreateInfo.flags |= ::vk::ShaderCreateFlagBitsEXT::eNoTaskShader;
            }

            // TODO push constant range
            //
            shaderCreateInfos.push_back(soCreateInfo);
        }

        auto [result, shaderObjects] = getHandle().createShadersEXT(shaderCreateInfos, vk_allocator());
        VK_VR(result);

        for (size_t idx = 0; idx < shaders.size(); ++idx)
        {
            APH_VR(setDebugObjectName(shaderObjects[idx], std::format("shader object: [{}]", idx)));
            shaderObjectMaps[shaders[idx]->getStage()] = shaderObjects[idx];
        }
    }

    *ppProgram = m_resourcePool.program.allocate(createInfo, shaderObjectMaps);

    return Result::Success;
}

Result Device::createImpl(const ImageViewCreateInfo& createInfo, ImageView** ppImageView)
{
    APH_PROFILER_SCOPE();
    ::vk::ImageViewCreateInfo info{};
    info.setImage(createInfo.pImage->getHandle())
        .setViewType(utils::VkCast(createInfo.viewType))
        .setFormat(utils::VkCast(createInfo.format));

    info.subresourceRange.setAspectMask(utils::getImageAspect(createInfo.format))
        .setLayerCount(createInfo.subresourceRange.layerCount)
        .setLevelCount(createInfo.subresourceRange.levelCount)
        .setBaseArrayLayer(createInfo.subresourceRange.baseArrayLayer)
        .setBaseMipLevel(createInfo.subresourceRange.baseMipLevel);

    memcpy(&info.components, &createInfo.components, sizeof(VkComponentMapping));

    auto [result, handle] = getHandle().createImageView(info, vk_allocator());
    VK_VR(result);

    *ppImageView = m_resourcePool.imageView.allocate(createInfo, handle);

    return Result::Success;
}

Result Device::createImpl(const BufferCreateInfo& createInfo, Buffer** ppBuffer)
{
    APH_PROFILER_SCOPE();
    // create buffer
    ::vk::BufferCreateInfo bufferInfo{};
    bufferInfo.setSize(createInfo.size)
        .setUsage(utils::VkCast(createInfo.usage | BufferUsage::ShaderDeviceAddress))
        .setSharingMode(::vk::SharingMode::eExclusive);
    auto [result, buffer] = getHandle().createBuffer(bufferInfo, vk_allocator());
    *ppBuffer = m_resourcePool.buffer.allocate(createInfo, buffer);
    m_resourcePool.deviceMemory->allocate(*ppBuffer);

    return Result::Success;
}

Result Device::createImpl(const ImageCreateInfo& createInfo, Image** ppImage)
{
    APH_PROFILER_SCOPE();
    ::vk::ImageCreateInfo imageCreateInfo{};
    auto [usage, flags] = utils::VkCast(createInfo.usage);
    imageCreateInfo.setFlags(flags)
        .setImageType(utils::VkCast(createInfo.imageType))
        .setFormat(utils::VkCast(createInfo.format))
        .setMipLevels(createInfo.mipLevels)
        .setArrayLayers(createInfo.arraySize)
        .setSamples(utils::getSampleCountFlags(createInfo.sampleCount))
        .setTiling(::vk::ImageTiling::eOptimal)
        .setUsage(usage)
        .setSharingMode(::vk::SharingMode::eExclusive)
        .setInitialLayout(::vk::ImageLayout::eUndefined);

    imageCreateInfo.extent.width = createInfo.extent.width;
    imageCreateInfo.extent.height = createInfo.extent.height;
    imageCreateInfo.extent.depth = createInfo.extent.depth;

    auto [result, image] = getHandle().createImage(imageCreateInfo, vk_allocator());
    VK_VR(result);
    *ppImage = m_resourcePool.image.allocate(this, createInfo, image);
    m_resourcePool.deviceMemory->allocate(*ppImage);

    return Result::Success;
}

void Device::destroyImpl(DescriptorSetLayout* pSetLayout)
{
    APH_PROFILER_SCOPE();
    getHandle().destroyDescriptorSetLayout(pSetLayout->getHandle(), vk_allocator());
    m_resourcePool.setLayout.free(pSetLayout);
}

void Device::destroyImpl(ShaderProgram* pProgram)
{
    APH_PROFILER_SCOPE();

    destroy(pProgram->getPipelineLayout());

    for (auto [_, shaderObject] : pProgram->m_shaderObjects)
    {
        getHandle().destroyShaderEXT(shaderObject, vk_allocator());
    }

    m_resourcePool.program.free(pProgram);
}

void Device::destroyImpl(Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->free(pBuffer);
    getHandle().destroyBuffer(pBuffer->getHandle(), vk_allocator());
    m_resourcePool.buffer.free(pBuffer);
}

void Device::destroyImpl(Image* pImage)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->free(pImage);
    getHandle().destroyImage(pImage->getHandle(), vk_allocator());
    m_resourcePool.image.free(pImage);
}

void Device::destroyImpl(ImageView* pImageView)
{
    APH_PROFILER_SCOPE();
    getHandle().destroyImageView(pImageView->getHandle(), vk_allocator());
    m_resourcePool.imageView.free(pImageView);
}

Result Device::createImpl(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain)
{
    APH_PROFILER_SCOPE();
    *ppSwapchain = new SwapChain(createInfo, this);
    return Result::Success;
}

void Device::destroyImpl(SwapChain* pSwapchain)
{
    APH_PROFILER_SCOPE();
    getHandle().destroySwapchainKHR(pSwapchain->getHandle(), vk_allocator());
    delete pSwapchain;
    pSwapchain = nullptr;
}

Queue* Device::getQueue(QueueType type, uint32_t queueIndex)
{
    APH_PROFILER_SCOPE();

    if (m_queues.count(type) && queueIndex < m_queues[type].size() && m_queues[type][queueIndex] != nullptr)
    {
        return m_queues[type][queueIndex];
    }

    const QueueType fallbackOrder[] = { QueueType::Transfer, QueueType::Compute, QueueType::Graphics };

    for (QueueType fallbackType : fallbackOrder)
    {
        if (queueIndex < m_queues[fallbackType].size() && m_queues[fallbackType][queueIndex] != nullptr)
        {
            // CM_LOG_WARN("Requested queue type [%s] (index %u) not available. Falling back to queue type %d.",
            //             aph::vk::utils::toString(type), queueIndex, aph::vk::utils::toString(fallbackType));
            return m_queues[fallbackType][queueIndex];
        }
    }

    if (type != QueueType::Graphics && type != QueueType::Compute && type != QueueType::Transfer)
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

Result Device::waitIdle()
{
    APH_PROFILER_SCOPE();
    return utils::getResult(getHandle().waitIdle());
}

Result Device::waitForFence(ArrayProxy<Fence*> fences, bool waitAll, uint32_t timeout)
{
    APH_PROFILER_SCOPE();
    SmallVector<::vk::Fence> vkFences(fences.size());
    for (auto idx = 0; idx < fences.size(); ++idx)
    {
        vkFences[idx] = fences[idx]->getHandle();
    }
    return utils::getResult(getHandle().waitForFences(vkFences, waitAll, UINT64_MAX));
}

Result Device::flushMemory(Buffer* pBuffer, Range range)
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->flush(pBuffer, range);
}
Result Device::invalidateMemory(Buffer* pBuffer, Range range)
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->invalidate(pBuffer, range);
}

Result Device::flushMemory(Image* pImage, Range range)
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->flush(pImage, range);
}

Result Device::invalidateMemory(Image* pImage, Range range)
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->invalidate(pImage, range);
}

void* Device::mapMemory(Buffer* pBuffer) const
{
    APH_PROFILER_SCOPE();
    void* pMapped = {};
    auto result = m_resourcePool.deviceMemory->map(pBuffer, &pMapped);
    if (!result.success())
    {
        return nullptr;
    }
    return pMapped;
}

void Device::unMapMemory(Buffer* pBuffer) const
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->unMap(pBuffer);
}

Result Device::createImpl(const CommandPoolCreateInfo& createInfo, CommandPool** ppCommandPool)
{
    APH_PROFILER_SCOPE();
    ::vk::CommandPoolCreateInfo vkCreateInfo{};
    vkCreateInfo.setQueueFamilyIndex(createInfo.queue->getFamilyIndex());
    if (createInfo.transient)
    {
        vkCreateInfo.setFlags(::vk::CommandPoolCreateFlagBits::eTransient);
    }
    vkCreateInfo.setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    auto [res, pool] = getHandle().createCommandPool(vkCreateInfo, vk_allocator());
    VK_VR(res);
    *ppCommandPool = m_resourcePool.commandPool.allocate(this, createInfo, pool);
    return Result::Success;
}

Result Device::createImpl(const SamplerCreateInfo& createInfo, Sampler** ppSampler)
{
    APH_PROFILER_SCOPE();

    // default sampler lod values
    // used if not overriden by mSetLodRange or not Linear mipmaps
    float minSamplerLod = 0;
    float maxSamplerLod = createInfo.mipMapMode == SamplerMipmapMode::Linear ? ::vk::LodClampNone : 0;
    // user provided lods
    if (createInfo.setLodRange)
    {
        minSamplerLod = createInfo.minLod;
        maxSamplerLod = createInfo.maxLod;
    }

    ::vk::SamplerCreateInfo ci{};
    {
        ci.magFilter = utils::VkCast(createInfo.magFilter);
        ci.minFilter = utils::VkCast(createInfo.minFilter);
        ci.mipmapMode = utils::VkCast(createInfo.mipMapMode);
        ci.addressModeU = utils::VkCast(createInfo.addressU);
        ci.addressModeV = utils::VkCast(createInfo.addressV);
        ci.addressModeW = utils::VkCast(createInfo.addressW);
        ci.mipLodBias = createInfo.mipLodBias;
        ci.anisotropyEnable = (createInfo.maxAnisotropy > 0.0f && getEnabledFeatures().samplerAnisotropy);
        ci.maxAnisotropy = createInfo.maxAnisotropy;
        ci.compareEnable = createInfo.compareFunc != CompareOp::Never;
        ci.compareOp = utils::VkCast(createInfo.compareFunc);
        ci.minLod = minSamplerLod;
        ci.maxLod = maxSamplerLod;
        ci.borderColor = ::vk::BorderColor::eFloatTransparentBlack;
        ci.unnormalizedCoordinates = ::vk::False;
    }

    auto [result, sampler] = getHandle().createSampler(ci, vk_allocator());
    VK_VR(result);
    *ppSampler = m_resourcePool.sampler.allocate(this, createInfo, sampler);
    return Result::Success;
}

void Device::destroyImpl(CommandPool* pPool)
{
    APH_PROFILER_SCOPE();
    pPool->reset(true);
    getHandle().destroyCommandPool(pPool->getHandle(), vk_allocator());
    m_resourcePool.commandPool.free(pPool);
}

void Device::destroyImpl(Sampler* pSampler)
{
    APH_PROFILER_SCOPE();
    getHandle().destroySampler(pSampler->getHandle(), vk_allocator());
    m_resourcePool.sampler.free(pSampler);
}

double Device::getTimeQueryResults(::vk::QueryPool pool, uint32_t firstQuery, uint32_t secondQuery, TimeUnit unitType)
{
    APH_PROFILER_SCOPE();
    uint64_t firstTimeStamp, secondTimeStamp;

    auto res = getHandle().getQueryPoolResults(pool, firstQuery, 1, sizeof(uint64_t), &firstTimeStamp, sizeof(uint64_t),
                                               ::vk::QueryResultFlagBits::e64 | ::vk::QueryResultFlagBits::eWait);
    VK_VR(res);
    res = getHandle().getQueryPoolResults(pool, secondQuery, 1, sizeof(uint64_t), &secondTimeStamp, sizeof(uint64_t),
                                          ::vk::QueryResultFlagBits::e64 | ::vk::QueryResultFlagBits::eWait);
    VK_VR(res);

    uint64_t timeDifference = secondTimeStamp - firstTimeStamp;
    auto period = getPhysicalDevice()->getProperties().timestampPeriod;
    auto timeInSeconds = timeDifference * period;

    switch (unitType)
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
    APH_VR(m_resourcePool.syncPrimitive.acquireSemaphore(1, &semaphore));
    return semaphore;
}
Result Device::releaseSemaphore(Semaphore* semaphore)
{
    APH_PROFILER_SCOPE();
    if (semaphore != VK_NULL_HANDLE)
    {
        auto result = m_resourcePool.syncPrimitive.ReleaseSemaphores(1, &semaphore);
        if (!result.success())
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
    APH_VR(m_resourcePool.syncPrimitive.acquireFence(&pFence, isSignaled));
    return pFence;
}
Result Device::releaseFence(Fence* pFence)
{
    APH_PROFILER_SCOPE();
    auto res = m_resourcePool.syncPrimitive.releaseFence(pFence);
    if (!res.success())
    {
        return Result::RuntimeError;
    }
    return Result::Success;
}

void Device::executeCommand(Queue* queue, const CmdRecordCallBack&& func, ArrayProxy<Semaphore*> waitSems,
                            ArrayProxy<Semaphore*> signalSems, Fence* pFence)
{
    APH_PROFILER_SCOPE();

    CommandPool* commandPool = {};
    APH_VR(create(CommandPoolCreateInfo{ .queue = queue, .transient = true }, &commandPool));

    CommandBuffer* cmd = nullptr;
    APH_VR(commandPool->allocate(1, &cmd));

    APH_VR(cmd->begin());
    func(cmd);
    APH_VR(cmd->end());

    QueueSubmitInfo submitInfo{ .commandBuffers = { cmd }, .waitSemaphores = waitSems, .signalSemaphores = signalSems };

    if (!pFence)
    {
        auto fence = acquireFence(false);
        APH_VR(queue->submit({ submitInfo }, fence));
        fence->wait();
    }
    else
    {
        APH_VR(queue->submit({ submitInfo }, pFence));
        // TODO async with caller
        pFence->wait();
    }

    destroy(commandPool);
}

::vk::PipelineStageFlags Device::determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType)
{
    APH_PROFILER_SCOPE();
    ::vk::PipelineStageFlags flags = {};

    const auto& features = getEnabledFeatures();

    switch (queueType)
    {
    case aph::QueueType::Graphics:
    {
        if ((accessFlags & (::vk::AccessFlagBits::eIndexRead | ::vk::AccessFlagBits::eVertexAttributeRead)))
        {
            flags |= ::vk::PipelineStageFlagBits::eVertexInput;
        }

        if ((accessFlags & (::vk::AccessFlagBits::eUniformRead | ::vk::AccessFlagBits::eShaderRead |
                            ::vk::AccessFlagBits::eShaderWrite)))
        {
            flags |= ::vk::PipelineStageFlagBits::eVertexShader;
            flags |= ::vk::PipelineStageFlagBits::eFragmentShader;
            if (features.tessellationSupported)
            {
                flags |= ::vk::PipelineStageFlagBits::eTessellationControlShader;
                flags |= ::vk::PipelineStageFlagBits::eTessellationEvaluationShader;
            }
            flags |= ::vk::PipelineStageFlagBits::eComputeShader;

            if (features.rayTracing)
            {
                flags |= ::vk::PipelineStageFlagBits::eRayTracingShaderKHR;
            }
        }

        if ((accessFlags & ::vk::AccessFlagBits::eInputAttachmentRead))
        {
            flags |= ::vk::PipelineStageFlagBits::eFragmentShader;
        }

        if ((accessFlags & (::vk::AccessFlagBits::eColorAttachmentRead | ::vk::AccessFlagBits::eColorAttachmentWrite)))
        {
            flags |= ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
        }

        if ((accessFlags &
             (::vk::AccessFlagBits::eDepthStencilAttachmentRead | ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)))
        {
            flags |= ::vk::PipelineStageFlagBits::eEarlyFragmentTests | ::vk::PipelineStageFlagBits::eLateFragmentTests;
        }

        break;
    }
    case aph::QueueType::Compute:
    {
        if ((accessFlags & (::vk::AccessFlagBits::eIndexRead | ::vk::AccessFlagBits::eVertexAttributeRead)) ||
            (accessFlags & ::vk::AccessFlagBits::eInputAttachmentRead) ||
            (accessFlags &
             (::vk::AccessFlagBits::eColorAttachmentRead | ::vk::AccessFlagBits::eColorAttachmentWrite)) ||
            (accessFlags &
             (::vk::AccessFlagBits::eDepthStencilAttachmentRead | ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)))
        {
            return ::vk::PipelineStageFlagBits::eAllCommands;
        }

        if ((accessFlags & (::vk::AccessFlagBits::eUniformRead | ::vk::AccessFlagBits::eShaderRead |
                            ::vk::AccessFlagBits::eShaderWrite)))
        {
            flags |= ::vk::PipelineStageFlagBits::eComputeShader;
        }

        break;
    }
    case aph::QueueType::Transfer:
        return ::vk::PipelineStageFlagBits::eAllCommands;
    default:
        break;
    }

    // Compatible with both compute and graphics queues
    if ((accessFlags & ::vk::AccessFlagBits::eIndirectCommandRead))
    {
        flags |= ::vk::PipelineStageFlagBits::eDrawIndirect;
    }

    if ((accessFlags & (::vk::AccessFlagBits::eTransferRead | ::vk::AccessFlagBits::eTransferWrite)))
    {
        flags |= ::vk::PipelineStageFlagBits::eTransfer;
    }

    if ((accessFlags & (::vk::AccessFlagBits::eHostRead | ::vk::AccessFlagBits::eHostWrite)))
    {
        flags |= ::vk::PipelineStageFlagBits::eHost;
    }

    if (!flags)
    {
        flags = ::vk::PipelineStageFlagBits::eTopOfPipe;
    }

    return flags;
}

static RENDERDOC_API_1_6_0* rdcDispatchTable = {};

void Device::begineCapture()
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->StartFrameCapture({}, {});
    }
}

void Device::endCapture()
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->EndFrameCapture({}, {});
    }
}

Result Device::initCapture()
{
    m_renderdocModule.open("librenderdoc.so");
    if (!m_renderdocModule)
    {
        return { Result::RuntimeError, "Failed to loading renderdoc module." };
    }

    pRENDERDOC_GetAPI getAPI = m_renderdocModule.getSymbol<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");
    if (!getAPI)
    {
        return { Result::RuntimeError, "Failed to get module symbol." };
    }

    if (!getAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdcDispatchTable))
    {
        return { Result::RuntimeError, "Failed to get dispatch table." };
    }

    return Result::Success;
}
void Device::triggerCapture()
{
    if (rdcDispatchTable)
    {
        rdcDispatchTable->TriggerCapture();
    }
}
DeviceAddress Device::getDeviceAddress(Buffer* pBuffer) const
{
    ::vk::DeviceAddress address = getHandle().getBufferAddress(::vk::BufferDeviceAddressInfo{ pBuffer->getHandle() });
    return static_cast<DeviceAddress>(address);
}
BindlessResource* Device::getBindlessResource() const
{
    APH_ASSERT(m_resourcePool.bindless);
    return m_resourcePool.bindless.get();
}
PhysicalDevice* Device::getPhysicalDevice() const
{
    return getCreateInfo().pPhysicalDevice;
}
GPUFeature Device::getEnabledFeatures() const
{
    return getCreateInfo().enabledFeatures;
}
Result Device::createImpl(const PipelineLayoutCreateInfo& createInfo, PipelineLayout** ppLayout)
{
    APH_PROFILER_SCOPE();

    ::vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    SmallVector<::vk::DescriptorSetLayout> vkSetLayouts;
    for (auto* setLayout : createInfo.setLayouts)
    {
        vkSetLayouts.push_back(setLayout->getHandle());
    }
    pipelineLayoutCreateInfo.setSetLayouts(vkSetLayouts);

    auto [result, handle] = getHandle().createPipelineLayout(pipelineLayoutCreateInfo, vk_allocator());
    VK_VR(result);
    *ppLayout = m_resourcePool.pipelineLayout.allocate(createInfo, handle);
    return Result::Success;
}
void Device::destroyImpl(PipelineLayout* pLayout)
{
    APH_PROFILER_SCOPE();

    for (auto* setLayout : pLayout->getSetLayouts())
    {
        destroy(setLayout);
    }

    getHandle().destroyPipelineLayout(pLayout->getHandle(), vk_allocator());
    m_resourcePool.pipelineLayout.free(pLayout);
}
} // namespace aph::vk
