#include "device.h"
#include "bindless.h"
#include "vmaAllocator.h"

#include "common/profiler.h"
#include "exception/errorMacros.h"

const VkAllocationCallbacks* gVkAllocator = aph::vk::vkAllocator();

namespace aph::vk
{
Device::Device(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo)
    , m_resourcePool(this)
{
}

auto Device::Create(const DeviceCreateInfo& createInfo) -> Expected<Device*>
{
    APH_PROFILER_SCOPE();

    // Create the device with minimal initialization
    PhysicalDevice* gpu = createInfo.pPhysicalDevice;
    APH_ASSERT(gpu, "PhysicalDevice cannot be null");
    if (gpu == nullptr)
    {
        return { Result::ArgumentOutOfRange, "PhysicalDevice is null" };
    }

    // Create device instance first with just a placeholder handle
    auto* pDevice = new Device(createInfo, {});
    APH_ASSERT(pDevice, "Failed to allocate Device instance");
    if (pDevice == nullptr)
    {
        return { Result::RuntimeError, "Failed to allocate Device instance" };
    }

    // Complete initialization
    Result initResult = pDevice->initialize(createInfo);
    if (!initResult.success())
    {
        delete pDevice;
        return { initResult.getCode(), initResult.toString() };
    }

    return pDevice;
}

auto Device::initialize(const DeviceCreateInfo& createInfo) -> Result
{
    APH_PROFILER_SCOPE();
    PhysicalDevice* gpu = createInfo.pPhysicalDevice;

    // Setup structure to hold our queue configurations and device resources
    const auto& queueFamilyProperties = gpu->getHandle().getQueueFamilyProperties();
    const auto queueFamilyCount       = queueFamilyProperties.size();
    SmallVector<::vk::DeviceQueueCreateInfo> queueCreateInfos(queueFamilyCount);
    SmallVector<SmallVector<float>> priorities(queueFamilyCount);
    SmallVector<const char*> requiredExtensions;
    ::vk::PhysicalDeviceFeatures2 supportedFeatures2;
    ::vk::DeviceCreateInfo deviceCreateInfo{};
    ::vk::Queue queue;
    ::vk::Device deviceHandle;

    //
    // 1. Set up queue family configuration
    //
    {
        for (auto i = 0U; i < queueFamilyCount; ++i)
        {
            const float defaultPriority = 1.0F;
            priorities[i].resize(queueFamilyProperties[i].queueCount, defaultPriority);
            queueCreateInfos[i]
                .setQueueFamilyIndex(i)
                .setQueueCount(queueFamilyProperties[i].queueCount)
                .setPQueuePriorities(priorities[i].data());
        }
    }

    //
    // 2. Configure, validate and setup device features & extensions
    //
    {
        // Setup extensions and enable features based on requirements
        const auto& features = createInfo.enabledFeatures;
        gpu->setupRequiredExtensions(features, requiredExtensions);
        gpu->enableFeatures(features);

        // Check and report any unsupported extensions
        bool allExtensionsSupported = true;
        SmallVector<std::string> unsupportedExtensions;

        for (const auto& requiredExtension : requiredExtensions)
        {
            if (!gpu->checkExtensionSupported(requiredExtension))
            {
                unsupportedExtensions.push_back(requiredExtension);
                allExtensionsSupported = false;
            }
        }

        if (!allExtensionsSupported)
        {
            VK_LOG_ERR("Required device extensions not supported:");
            for (const auto& ext : unsupportedExtensions)
            {
                VK_LOG_ERR("  - %s", ext.c_str());
            }
            return { Result::RuntimeError, "Required device extensions not supported" };
        }

        // Validate hardware feature support and get feature entry names for any unsupported features
        if (!gpu->validateFeatures(features))
        {
            VK_LOG_ERR("Critical GPU features not supported by hardware");
            // The validateFeatures already logs which features are unsupported
            return { Result::RuntimeError, "Critical GPU features not supported by hardware" };
        }

        // Setup physical device features for device creation
        supportedFeatures2 = gpu->getHandle().getFeatures2();

        // Enable specific required features
        supportedFeatures2.features.sampleRateShading = VK_TRUE;
        supportedFeatures2.features.samplerAnisotropy = VK_TRUE;

        supportedFeatures2.setPNext(gpu->getRequestedFeatures());
    }

    //
    // 3. Create the logical device
    //
    {
        deviceCreateInfo.setPNext(&supportedFeatures2)
            .setQueueCreateInfos(queueCreateInfos)
            .setPEnabledExtensionNames(requiredExtensions);

        auto [result, handle] = gpu->getHandle().createDevice(deviceCreateInfo, vk_allocator());
        if (result != ::vk::Result::eSuccess)
        {
            return { Result::RuntimeError, "Failed to create logical device" };
        }

        // Store the handle in the device object
        m_handle = handle;
    }

    //
    // 4. Initialize device and core resources
    //
    {
        m_resourcePool.deviceMemory           = std::make_unique<VMADeviceAllocator>(createInfo.pInstance, this);
        m_resourcePool.bindless               = std::make_unique<BindlessResource>(this);
        m_resourcePool.commandBufferAllocator = std::make_unique<CommandBufferAllocator>(this);
        m_resourcePool.samplerPool            = std::make_unique<SamplerPool>(this);
        m_resourcePool.queryPoolAllocator     = std::make_unique<QueryPoolAllocator>(this);

        // Initialize the sampler pool
        auto result = m_resourcePool.samplerPool->initialize();
        if (!result.success())
        {
            return { Result::RuntimeError, "Failed to initialize sampler pool" };
        }

        // Initialize query pools
        QueryPoolAllocationConfig queryPoolConfig{};
        result = m_resourcePool.queryPoolAllocator->initialize(queryPoolConfig);
        if (!result.success())
        {
            return { Result::RuntimeError, "Failed to initialize query pools" };
        }
    }

    //
    // 5. Initialize device queues
    //
    {
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
        {
            auto& queueFamily = queueFamilyProperties[queueFamilyIndex];
            auto queueFlags   = queueFamily.queueFlags;

            // Determine queue type based on capabilities
            QueueType queueType = QueueType::Unsupport;

            if (queueFlags & ::vk::QueueFlagBits::eGraphics)
            {
                VK_LOG_DEBUG("create graphics queue %lu", queueFamilyIndex);
                queueType = QueueType::Graphics;
            }
            else if (queueFlags & ::vk::QueueFlagBits::eCompute)
            {
                VK_LOG_DEBUG("Found compute queue %lu", queueFamilyIndex);
                queueType = QueueType::Compute;
            }
            else if (queueFlags & ::vk::QueueFlagBits::eTransfer)
            {
                VK_LOG_DEBUG("Found transfer queue %lu", queueFamilyIndex);
                queueType = QueueType::Transfer;
            }

            // Create all queues for this family
            for (auto queueIndex = 0U; queueIndex < queueCreateInfos[queueFamilyIndex].queueCount; ++queueIndex)
            {
                ::vk::DeviceQueueInfo2 queueInfo{};
                queueInfo.setQueueFamilyIndex(queueFamilyIndex).setQueueIndex(queueIndex);

                queue = m_handle.getQueue2(queueInfo);
                m_queues[queueType].push_back(
                    m_resourcePool.queue.allocate(queue, queueFamilyIndex, queueIndex, queueType));
            }
        }
    }

    return Result::Success;
}

auto Device::Destroy(Device* pDevice) -> void
{
    APH_ASSERT(pDevice);

    APH_PROFILER_SCOPE();

    APH_VERIFY_RESULT(pDevice->waitIdle());

    pDevice->m_resourcePool.commandBufferAllocator.reset();
    pDevice->m_resourcePool.bindless.reset();
    pDevice->m_resourcePool.program.clear();
    pDevice->m_resourcePool.syncPrimitive.clear();
    pDevice->m_resourcePool.setLayout.clear();
    pDevice->m_resourcePool.samplerPool.reset();
    pDevice->m_resourcePool.queryPoolAllocator.reset();
    pDevice->m_resourcePool.deviceMemory.reset();

    APH_ASSERT(pDevice->m_handle);
    pDevice->getHandle().destroy(vk_allocator());

    if (pDevice->getCreateInfo().enableDebug)
    {
        CM_LOG_INFO("Resource Tracking Report:\n%s", pDevice->getResourceStatsReport().c_str());
    }

    delete pDevice;
}

auto Device::getDepthFormat() const -> Format
{
    APH_PROFILER_SCOPE();
    Format format = getPhysicalDevice()->findSupportedFormat({ Format::D32, Format::D32S8, Format::D24S8 },
                                                             ::vk::ImageTiling::eOptimal,
                                                             ::vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    return format;
}

auto Device::createImpl(const DescriptorSetLayoutCreateInfo& createInfo) -> Expected<DescriptorSetLayout*>
{
    APH_PROFILER_SCOPE();
    const SmallVector<::vk::DescriptorSetLayoutBinding>& vkBindings = createInfo.bindings;
    const SmallVector<::vk::DescriptorPoolSize>& poolSizes          = createInfo.poolSizes;

    APH_ASSERT(!vkBindings.empty(), "Descriptor set layout bindings cannot be empty");

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
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create descriptor set layout" };
    }

    DescriptorSetLayout* pLayout =
        m_resourcePool.setLayout.allocate(this, createInfo, vkSetLayout, poolSizes, vkBindings);
    APH_ASSERT(pLayout, "Failed to allocate descriptor set layout");
    return Expected<DescriptorSetLayout*>{ pLayout };
}

auto Device::createImpl(const ProgramCreateInfo& createInfo) -> Expected<ShaderProgram*>
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(createInfo.pPipelineLayout, "Pipeline layout cannot be null");

    // Validate shaders map
    APH_ASSERT(!createInfo.shaders.empty(), "Shader map cannot be empty");

    // Setup variables needed for program creation
    bool hasTaskShader = false;
    SmallVector<Shader*> shaders{};
    SmallVector<::vk::DescriptorSetLayout> vkSetLayouts{};
    SmallVector<::vk::ShaderCreateInfoEXT> shaderCreateInfos{};
    HashMap<ShaderStage, ::vk::ShaderEXT> shaderObjectMaps{};

    //
    // 1. Collect shaders based on shader stage combination
    //
    {
        // Determine pipeline type first, which validates the shader combination
        PipelineType pipelineType = utils::determinePipelineType(createInfo.shaders);
        APH_ASSERT(pipelineType != PipelineType::Undefined, "Invalid shader stage combination");

        // Graphics pipeline: Vertex + Fragment
        if (pipelineType == PipelineType::Geometry)
        {
            APH_ASSERT(createInfo.shaders.contains(ShaderStage::VS), "Vertex shader required for geometry pipeline");
            APH_ASSERT(createInfo.shaders.contains(ShaderStage::FS), "Fragment shader required for geometry pipeline");
            shaders.push_back(createInfo.shaders.at(ShaderStage::VS));
            shaders.push_back(createInfo.shaders.at(ShaderStage::FS));
        }
        // Mesh pipeline: [Task] + Mesh + Fragment
        else if (pipelineType == PipelineType::Mesh)
        {
            APH_ASSERT(createInfo.shaders.contains(ShaderStage::MS), "Mesh shader required for mesh pipeline");
            APH_ASSERT(createInfo.shaders.contains(ShaderStage::FS), "Fragment shader required for mesh pipeline");

            if (createInfo.shaders.contains(ShaderStage::TS))
            {
                shaders.push_back(createInfo.shaders.at(ShaderStage::TS));
                hasTaskShader = true;
            }
            shaders.push_back(createInfo.shaders.at(ShaderStage::MS));
            shaders.push_back(createInfo.shaders.at(ShaderStage::FS));
        }
        // Compute pipeline: Compute
        else if (pipelineType == PipelineType::Compute)
        {
            APH_ASSERT(createInfo.shaders.contains(ShaderStage::CS), "Compute shader required for compute pipeline");
            shaders.push_back(createInfo.shaders.at(ShaderStage::CS));
        }
    }

    // Validate that shaders were collected
    APH_ASSERT(!shaders.empty(), "No valid shaders found in createInfo");

    //
    // 2. Collect descriptor set layouts from pipeline layout
    //
    {
        for (auto setLayout : createInfo.pPipelineLayout->getSetLayouts())
        {
            vkSetLayouts.push_back(setLayout->getHandle());
        }
    }

    //
    // 3. Create shader object infos
    //
    {
        shaderCreateInfos.reserve(shaders.size());

        for (auto iter = shaders.cbegin(); iter != shaders.cend(); ++iter)
        {
            auto* shader = *iter;

            // Set next stage if this isn't the last shader
            ::vk::ShaderStageFlags nextStage = {};
            if (auto nextIter = std::next(iter); nextIter != shaders.cend())
            {
                nextStage = utils::VkCast((*nextIter)->getStage());
            }

            // Configure shader creation parameters
            ::vk::ShaderCreateInfoEXT soCreateInfo{};
            soCreateInfo.setFlags(::vk::ShaderCreateFlagBitsEXT::eLinkStage)
                .setStage(utils::VkCast(shader->getStage()))
                .setNextStage(nextStage)
                .setPCode(shader->getCode().data())
                .setCodeSize(shader->getCode().size() * sizeof(shader->getCode()[0]))
                .setCodeType(::vk::ShaderCodeTypeEXT::eSpirv)
                .setPName(shader->getEntryPointName().data())
                .setSetLayouts(vkSetLayouts);

            // Configure mesh shader without task shader if needed
            if (!hasTaskShader && soCreateInfo.stage == ::vk::ShaderStageFlagBits::eMeshEXT)
            {
                soCreateInfo.flags |= ::vk::ShaderCreateFlagBitsEXT::eNoTaskShader;
            }

            shaderCreateInfos.push_back(soCreateInfo);
        }
    }

    //
    // 4. Create shader objects
    //
    {
        auto [result, shaderObjects] = getHandle().createShadersEXT(shaderCreateInfos, vk_allocator());
        if (result != ::vk::Result::eSuccess)
        {
            return { Result::RuntimeError, "Failed to create shader objects" };
        }

        // Map shader objects to their stages and set debug names
        for (size_t idx = 0; idx < shaders.size(); ++idx)
        {
            APH_VERIFY_RESULT(setDebugObjectName(shaderObjects[idx], std::format("shader object: [{}]", idx)));
            shaderObjectMaps[shaders[idx]->getStage()] = shaderObjects[idx];
        }
    }

    //
    // 5. Allocate program object
    //
    {
        ShaderProgram* pProgram = m_resourcePool.program.allocate(createInfo, shaderObjectMaps);
        return Expected<ShaderProgram*>{ pProgram };
    }
}

auto Device::createImpl(const ImageViewCreateInfo& createInfo) -> Expected<ImageView*>
{
    APH_PROFILER_SCOPE();

    // Validate image view parameters
    APH_ASSERT(createInfo.pImage, "Image cannot be null");
    APH_ASSERT(createInfo.format != Format::Undefined, "Image view format cannot be undefined");
    APH_ASSERT(createInfo.subresourceRange.layerCount > 0, "Image view must include at least one layer");
    APH_ASSERT(createInfo.subresourceRange.levelCount > 0, "Image view must include at least one mip level");

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
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create image view" };
    }

    ImageView* pImageView = m_resourcePool.imageView.allocate(createInfo, handle);
    APH_ASSERT(pImageView, "Failed to allocate image view from resource pool");

    return Expected<ImageView*>{ pImageView };
}

auto Device::createImpl(const BufferCreateInfo& createInfo) -> Expected<Buffer*>
{
    APH_PROFILER_SCOPE();

    // Validate buffer size
    APH_ASSERT(createInfo.size > 0, "Buffer size must be greater than 0");
    APH_ASSERT(createInfo.usage != BufferUsage::None, "Buffer must have at least one usage flag");

    // create buffer
    ::vk::BufferCreateInfo bufferInfo{};
    bufferInfo.setSize(createInfo.size)
        .setUsage(utils::VkCast(createInfo.usage | BufferUsage::ShaderDeviceAddress))
        .setSharingMode(::vk::SharingMode::eExclusive);
    auto [result, buffer] = getHandle().createBuffer(bufferInfo, vk_allocator());
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create buffer" };
    }

    Buffer* pBuffer = m_resourcePool.buffer.allocate(createInfo, buffer);
    APH_ASSERT(pBuffer, "Failed to allocate buffer from resource pool");

    auto* allocResult = m_resourcePool.deviceMemory->allocate(pBuffer);
    APH_ASSERT(allocResult, "Failed to allocate memory for buffer");

    return Expected<Buffer*>{ pBuffer };
}

auto Device::createImpl(const ImageCreateInfo& createInfo) -> Expected<Image*>
{
    APH_PROFILER_SCOPE();

    // Validate image parameters
    APH_ASSERT(createInfo.extent.width > 0, "Image width must be greater than 0");
    APH_ASSERT(createInfo.extent.height > 0, "Image height must be greater than 0");
    APH_ASSERT(createInfo.extent.depth > 0, "Image depth must be greater than 0");
    APH_ASSERT(createInfo.mipLevels > 0, "Image must have at least one mip level");
    APH_ASSERT(createInfo.arraySize > 0, "Image must have at least one array layer");
    APH_ASSERT(createInfo.format != Format::Undefined, "Image format cannot be undefined");
    APH_ASSERT(createInfo.usage != ImageUsage::None, "Image must have at least one usage flag");

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

    imageCreateInfo.extent.width  = createInfo.extent.width;
    imageCreateInfo.extent.height = createInfo.extent.height;
    imageCreateInfo.extent.depth  = createInfo.extent.depth;

    auto [result, image] = getHandle().createImage(imageCreateInfo, vk_allocator());
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create image" };
    }

    Image* pImage = m_resourcePool.image.allocate(this, createInfo, image);
    APH_ASSERT(pImage, "Failed to allocate image from resource pool");

    auto* allocResult = m_resourcePool.deviceMemory->allocate(pImage);
    APH_ASSERT(allocResult, "Failed to allocate memory for image");

    return Expected<Image*>{ pImage };
}

auto Device::destroyImpl(DescriptorSetLayout* pSetLayout) -> void
{
    APH_PROFILER_SCOPE();
    getHandle().destroyDescriptorSetLayout(pSetLayout->getHandle(), vk_allocator());
    m_resourcePool.setLayout.free(pSetLayout);
}

auto Device::destroyImpl(ShaderProgram* pProgram) -> void
{
    APH_PROFILER_SCOPE();

    destroy(pProgram->getPipelineLayout());

    for (auto [_, shaderObject] : pProgram->m_shaderObjects)
    {
        getHandle().destroyShaderEXT(shaderObject, vk_allocator());
    }

    m_resourcePool.program.free(pProgram);
}

auto Device::destroyImpl(Buffer* pBuffer) -> void
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->free(pBuffer);
    getHandle().destroyBuffer(pBuffer->getHandle(), vk_allocator());
    m_resourcePool.buffer.free(pBuffer);
}

auto Device::destroyImpl(Image* pImage) -> void
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->free(pImage);
    getHandle().destroyImage(pImage->getHandle(), vk_allocator());
    m_resourcePool.image.free(pImage);
}

auto Device::destroyImpl(ImageView* pImageView) -> void
{
    APH_PROFILER_SCOPE();
    getHandle().destroyImageView(pImageView->getHandle(), vk_allocator());
    m_resourcePool.imageView.free(pImageView);
}

auto Device::createImpl(const SwapChainCreateInfo& createInfo) -> Expected<SwapChain*>
{
    APH_PROFILER_SCOPE();
    auto* pSwapchain = new SwapChain(createInfo, this);
    return Expected<SwapChain*>{ pSwapchain };
}

auto Device::destroyImpl(SwapChain* pSwapchain) -> void
{
    APH_PROFILER_SCOPE();
    getHandle().destroySwapchainKHR(pSwapchain->getHandle(), vk_allocator());
    delete pSwapchain;
    pSwapchain = nullptr;
}

auto Device::getQueue(QueueType type, uint32_t queueIndex) -> Queue*
{
    APH_PROFILER_SCOPE();

    // Validate queue type
    APH_ASSERT(type == QueueType::Graphics || type == QueueType::Compute || type == QueueType::Transfer,
               "Invalid queue type requested");

    if (m_queues.contains(type) && queueIndex < m_queues[type].size() && m_queues[type][queueIndex] != nullptr)
    {
        return m_queues[type][queueIndex];
    }

    constexpr std::array fallbackOrder = { QueueType::Transfer, QueueType::Compute, QueueType::Graphics };

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

auto Device::waitIdle() -> Result
{
    APH_PROFILER_SCOPE();
    return utils::getResult(getHandle().waitIdle());
}

auto Device::waitForFence(ArrayProxy<Fence*> fences, bool waitAll, uint64_t timeout) -> Result
{
    APH_PROFILER_SCOPE();
    SmallVector<::vk::Fence> vkFences(fences.size());
    for (std::size_t idx = 0; idx < fences.size(); ++idx)
    {
        vkFences[idx] = fences[idx]->getHandle();
    }
    return utils::getResult(getHandle().waitForFences(vkFences, static_cast<::vk::Bool32>(waitAll), timeout));
}

auto Device::flushMemory(Buffer* pBuffer, Range range) const -> Result
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->flush(pBuffer, range);
}

auto Device::invalidateMemory(Buffer* pBuffer, Range range) const -> Result
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->invalidate(pBuffer, range);
}

auto Device::flushMemory(Image* pImage, Range range) const -> Result
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->flush(pImage, range);
}

auto Device::invalidateMemory(Image* pImage, Range range) const -> Result
{
    APH_PROFILER_SCOPE();
    return m_resourcePool.deviceMemory->invalidate(pImage, range);
}

auto Device::mapMemory(Buffer* pBuffer) const -> void*
{
    APH_PROFILER_SCOPE();

    // Validate buffer
    APH_ASSERT(pBuffer, "Cannot map null buffer");

    void* pMapped = {};
    auto result   = m_resourcePool.deviceMemory->map(pBuffer, &pMapped);
    if (!result.success())
    {
        return nullptr;
    }
    return pMapped;
}

auto Device::unMapMemory(Buffer* pBuffer) const -> void
{
    APH_PROFILER_SCOPE();

    // Validate buffer
    APH_ASSERT(pBuffer, "Cannot unmap null buffer");

    m_resourcePool.deviceMemory->unMap(pBuffer);
}

auto Device::destroyImpl(Sampler* pSampler) -> void
{
    APH_PROFILER_SCOPE();

    if (!pSampler)
        return;

    getHandle().destroySampler(pSampler->getHandle(), vk_allocator());
    m_resourcePool.sampler.free(pSampler);
}

auto Device::getTimeQueryResults(QueryPool* pQueryPool, uint32_t firstQuery, uint32_t secondQuery, TimeUnit unitType)
    -> double
{
    APH_PROFILER_SCOPE();

    // Validate query pool
    APH_ASSERT(pQueryPool != nullptr, "Query pool cannot be null");
    APH_ASSERT(pQueryPool->getQueryType() == QueryType::Timestamp, "Query pool must be of timestamp type");

    uint64_t firstTimeStamp  = 0;
    uint64_t secondTimeStamp = 0;

    auto res = getHandle().getQueryPoolResults(pQueryPool->getHandle(), firstQuery, 1, sizeof(uint64_t),
                                               &firstTimeStamp, sizeof(uint64_t),
                                               ::vk::QueryResultFlagBits::e64 | ::vk::QueryResultFlagBits::eWait);
    VK_VR(res);
    res = getHandle().getQueryPoolResults(pQueryPool->getHandle(), secondQuery, 1, sizeof(uint64_t), &secondTimeStamp,
                                          sizeof(uint64_t),
                                          ::vk::QueryResultFlagBits::e64 | ::vk::QueryResultFlagBits::eWait);
    VK_VR(res);

    uint64_t timeDifference = secondTimeStamp - firstTimeStamp;
    auto period             = getPhysicalDevice()->getProperties().timestampPeriod;
    uint64_t timeInSeconds  = timeDifference * period;

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

auto Device::acquireSemaphore() -> Semaphore*
{
    APH_PROFILER_SCOPE();
    Semaphore* semaphore = nullptr;
    APH_VERIFY_RESULT(m_resourcePool.syncPrimitive.acquireSemaphore(1, &semaphore));
    return semaphore;
}

auto Device::releaseSemaphore(Semaphore* semaphore) -> Result
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

auto Device::acquireFence(bool isSignaled) -> Fence*
{
    APH_PROFILER_SCOPE();
    Fence* pFence = {};
    APH_VERIFY_RESULT(m_resourcePool.syncPrimitive.acquireFence(&pFence, isSignaled));
    return pFence;
}

auto Device::releaseFence(Fence* pFence) -> Result
{
    APH_PROFILER_SCOPE();
    auto res = m_resourcePool.syncPrimitive.releaseFence(pFence);
    if (!res.success())
    {
        return Result::RuntimeError;
    }
    return Result::Success;
}

auto Device::executeCommand(Queue* queue, const CmdRecordCallBack&& func, ArrayProxy<Semaphore*> waitSems,
                            ArrayProxy<Semaphore*> signalSems, Fence* pFence) -> void
{
    APH_PROFILER_SCOPE();

    // Validate parameters
    APH_ASSERT(queue, "Queue cannot be null");
    APH_ASSERT(func, "Command record callback cannot be null");

    // Use the command buffer allocator instead of directly creaacquire
    CommandBuffer* cmd = m_resourcePool.commandBufferAllocator->acquire(queue->getType());
    APH_ASSERT(cmd, "Failed to acquire command buffer");

    APH_VERIFY_RESULT(cmd->begin());
    func(cmd);
    APH_VERIFY_RESULT(cmd->end());

    QueueSubmitInfo submitInfo{ .commandBuffers = { cmd }, .waitSemaphores = waitSems, .signalSemaphores = signalSems };

    Fence* fence   = pFence;
    bool ownsFence = false;

    if (fence == nullptr)
    {
        fence     = acquireFence(false);
        ownsFence = true;
    }

    APH_ASSERT(fence, "Failed to acquire fence");
    APH_VERIFY_RESULT(queue->submit({ submitInfo }, fence));
    fence->wait();

    // Release the command buffer back to the allocator after execution
    m_resourcePool.commandBufferAllocator->release(cmd);

    if (ownsFence)
    {
        APH_VERIFY_RESULT(releaseFence(fence));
    }
}

auto Device::determinePipelineStageFlags(::vk::AccessFlags accessFlags, QueueType queueType) -> ::vk::PipelineStageFlags
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

auto Device::destroyImpl(PipelineLayout* pLayout) -> void
{
    APH_PROFILER_SCOPE();

    for (auto* setLayout : pLayout->getSetLayouts())
    {
        destroy(setLayout);
    }

    getHandle().destroyPipelineLayout(pLayout->getHandle(), vk_allocator());
    m_resourcePool.pipelineLayout.free(pLayout);
}

auto Device::createImpl(const SamplerCreateInfo& createInfo) -> Expected<Sampler*>
{
    APH_PROFILER_SCOPE();

    // Only check for matching samplers once the sampler pool is fully initialized
    if (m_resourcePool.samplerPool)
    {
        Sampler* existingSampler = m_resourcePool.samplerPool->findMatchingSampler(createInfo);
        if (existingSampler)
        {
            return Expected<Sampler*>{ existingSampler };
        }
    }

    // Validate sampler parameters
    APH_ASSERT(createInfo.magFilter == Filter::Nearest || createInfo.magFilter == Filter::Linear ||
                   createInfo.magFilter == Filter::Cubic,
               "Invalid magnification filter");
    APH_ASSERT(createInfo.minFilter == Filter::Nearest || createInfo.minFilter == Filter::Linear ||
                   createInfo.minFilter == Filter::Cubic,
               "Invalid minification filter");

    // Configure LOD settings
    float minLod = 0.0f;
    float maxLod = createInfo.mipMapMode == SamplerMipmapMode::Linear ? ::vk::LodClampNone : 0.0f;

    if (createInfo.setLodRange)
    {
        minLod = createInfo.minLod;
        maxLod = createInfo.maxLod;
        APH_ASSERT(createInfo.maxLod >= createInfo.minLod, "Max LOD must be greater than or equal to Min LOD");
    }

    // Create the Vulkan sampler
    ::vk::SamplerCreateInfo ci{};
    ci.magFilter    = utils::VkCast(createInfo.magFilter);
    ci.minFilter    = utils::VkCast(createInfo.minFilter);
    ci.mipmapMode   = utils::VkCast(createInfo.mipMapMode);
    ci.addressModeU = utils::VkCast(createInfo.addressU);
    ci.addressModeV = utils::VkCast(createInfo.addressV);
    ci.addressModeW = utils::VkCast(createInfo.addressW);
    ci.mipLodBias   = createInfo.mipLodBias;
    ci.anisotropyEnable =
        static_cast<::vk::Bool32>(createInfo.maxAnisotropy > 0.0F && getEnabledFeatures().samplerAnisotropy);
    ci.maxAnisotropy           = createInfo.maxAnisotropy;
    ci.compareEnable           = static_cast<::vk::Bool32>(createInfo.compareFunc != CompareOp::Never);
    ci.compareOp               = utils::VkCast(createInfo.compareFunc);
    ci.minLod                  = minLod;
    ci.maxLod                  = maxLod;
    ci.borderColor             = ::vk::BorderColor::eFloatTransparentBlack;
    ci.unnormalizedCoordinates = ::vk::False;

    // Create the Vulkan sampler
    auto [result, samplerHandle] = getHandle().createSampler(ci, vk_allocator());
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create sampler" };
    }

    // Create the sampler object
    Sampler* pSampler = m_resourcePool.sampler.allocate(createInfo, samplerHandle);
    APH_ASSERT(pSampler, "Failed to allocate sampler from resource pool");

    return Expected<Sampler*>{ pSampler };
}

auto Device::getDeviceAddress(Buffer* pBuffer) const -> DeviceAddress
{
    ::vk::DeviceAddress address = getHandle().getBufferAddress(::vk::BufferDeviceAddressInfo{ pBuffer->getHandle() });
    return static_cast<DeviceAddress>(address);
}

auto Device::getBindlessResource() const -> BindlessResource*
{
    APH_ASSERT(m_resourcePool.bindless);
    return m_resourcePool.bindless.get();
}

auto Device::getPhysicalDevice() const -> PhysicalDevice*
{
    return getCreateInfo().pPhysicalDevice;
}

auto Device::getEnabledFeatures() const -> GPUFeature
{
    return getCreateInfo().enabledFeatures;
}

auto Device::createImpl(const PipelineLayoutCreateInfo& createInfo) -> Expected<PipelineLayout*>
{
    APH_PROFILER_SCOPE();

    // Validate parameters
    for (auto* setLayout : createInfo.setLayouts)
    {
        APH_ASSERT(setLayout, "Pipeline layout contains null descriptor set layout");
    }

    ::vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    SmallVector<::vk::DescriptorSetLayout> vkSetLayouts;
    for (auto* setLayout : createInfo.setLayouts)
    {
        vkSetLayouts.push_back(setLayout->getHandle());
    }
    pipelineLayoutCreateInfo.setSetLayouts(vkSetLayouts);

    auto [result, handle] = getHandle().createPipelineLayout(pipelineLayoutCreateInfo, vk_allocator());
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create pipeline layout" };
    }

    PipelineLayout* pLayout = m_resourcePool.pipelineLayout.allocate(createInfo, handle);
    APH_ASSERT(pLayout, "Failed to allocate pipeline layout from resource pool");

    return Expected<PipelineLayout*>{ pLayout };
}

auto Device::getCommandBufferAllocator() const -> CommandBufferAllocator*
{
    return m_resourcePool.commandBufferAllocator.get();
}

auto Device::getSampler(PresetSamplerType type) const -> Sampler*
{
    return m_resourcePool.samplerPool->getSampler(type);
}

auto Device::getSamplerPool() const -> SamplerPool*
{
    return m_resourcePool.samplerPool.get();
}

auto Device::getResourceStats() const -> const ResourceStats&
{
    return m_resourceStats;
}

auto Device::createImpl(const QueryPoolCreateInfo& createInfo) -> Expected<QueryPool*>
{
    APH_PROFILER_SCOPE();

    // Validate query pool parameters
    APH_ASSERT(createInfo.queryCount > 0, "Query count must be greater than 0");

    // Configure statistics flags only if needed
    ::vk::QueryPipelineStatisticFlags statisticsFlags;
    if (createInfo.type == QueryType::PipelineStatistics)
    {
        APH_ASSERT(createInfo.statisticsFlags != PipelineStatisticsFlags{},
                   "Pipeline statistics flags must be set for pipeline statistics query pool");
        statisticsFlags = utils::VkCast(createInfo.statisticsFlags);
    }

    // Create query pool
    ::vk::QueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.setQueryType(utils::VkCast(createInfo.type)).setQueryCount(createInfo.queryCount);

    if (createInfo.type == QueryType::PipelineStatistics)
    {
        queryPoolInfo.setPipelineStatistics(statisticsFlags);
    }

    auto [result, queryPool] = getHandle().createQueryPool(queryPoolInfo, vk_allocator());
    if (result != ::vk::Result::eSuccess)
    {
        return { Result::RuntimeError, "Failed to create query pool" };
    }

    QueryPool* pQueryPool = m_resourcePool.queryPool.allocate(createInfo, queryPool);
    APH_ASSERT(pQueryPool, "Failed to allocate query pool from resource pool");

    return Expected<QueryPool*>{ pQueryPool };
}

auto Device::destroyImpl(QueryPool* pQueryPool) -> void
{
    APH_PROFILER_SCOPE();

    if (!pQueryPool)
        return;

    getHandle().destroyQueryPool(pQueryPool->getHandle(), vk_allocator());
    m_resourcePool.queryPool.free(pQueryPool);
}

auto Device::acquireQueryPool(QueryType type) -> QueryPool*
{
    APH_PROFILER_SCOPE();

    APH_ASSERT(m_resourcePool.queryPoolAllocator, "Query pool allocator not initialized");

    return m_resourcePool.queryPoolAllocator->acquire(type);
}

auto Device::releaseQueryPool(QueryPool* pQueryPool) -> Result
{
    APH_PROFILER_SCOPE();

    if (pQueryPool != nullptr && m_resourcePool.queryPoolAllocator)
    {
        return m_resourcePool.queryPoolAllocator->release(pQueryPool);
    }

    return Result::Success;
}

auto Device::resetQueryPools(QueryType type, CommandBuffer* pCommandBuffer) -> void
{
    APH_PROFILER_SCOPE();

    APH_ASSERT(m_resourcePool.queryPoolAllocator, "Query pool allocator not initialized");
    APH_ASSERT(pCommandBuffer, "Command buffer cannot be null");

    m_resourcePool.queryPoolAllocator->resetAll(type, pCommandBuffer);
}
} // namespace aph::vk
