#include "device.h"
#include "bindless.h"
#include "common/profiler.h"
#include "deviceAllocator.h"
#include "module/module.h"
#include "renderdoc_app.h"
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

        // must support features
        {
            requiredExtensions.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
            requiredExtensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
            requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            requiredExtensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);

            auto& extDynamicState3 = gpu->requestFeatures<::vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT>();
            extDynamicState3.extendedDynamicState3ColorBlendEquation = VK_TRUE;

            auto& shaderObjectFeatures = gpu->requestFeatures<::vk::PhysicalDeviceShaderObjectFeaturesEXT>();
            shaderObjectFeatures.shaderObject = VK_TRUE;

            auto& sync2Features = gpu->requestFeatures<::vk::PhysicalDeviceSynchronization2Features>();
            sync2Features.synchronization2 = VK_TRUE;

            auto& timelineSemaphoreFeatures = gpu->requestFeatures<::vk::PhysicalDeviceTimelineSemaphoreFeatures>();
            timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

            auto& maintenance4Features = gpu->requestFeatures<::vk::PhysicalDeviceMaintenance4Features>();
            maintenance4Features.maintenance4 = VK_TRUE;

            // Request Inline Uniform Block Features EXT
            auto& inlineUniformBlockFeature = gpu->requestFeatures<::vk::PhysicalDeviceInlineUniformBlockFeaturesEXT>();
            inlineUniformBlockFeature.inlineUniformBlock = VK_TRUE;

            // Request Dynamic Rendering Features KHR
            auto& dynamicRenderingFeature = gpu->requestFeatures<::vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
            dynamicRenderingFeature.dynamicRendering = VK_TRUE;

            // Request Host Query Reset Features
            auto& hostQueryResetFeature = gpu->requestFeatures<::vk::PhysicalDeviceHostQueryResetFeatures>();
            hostQueryResetFeature.hostQueryReset = VK_TRUE;

            auto& deviceAddressFeatures = gpu->requestFeatures<::vk::PhysicalDeviceBufferDeviceAddressFeatures>();
            deviceAddressFeatures.setBufferDeviceAddress(::vk::True);
        }

        if (feature.meshShading)
        {
            // Request Mesh Shader Features EXT
            auto& meshShaderFeature = gpu->requestFeatures<::vk::PhysicalDeviceMeshShaderFeaturesEXT>();

            meshShaderFeature.taskShader = VK_TRUE;
            meshShaderFeature.meshShader = VK_TRUE;
            meshShaderFeature.meshShaderQueries = VK_FALSE;
            meshShaderFeature.multiviewMeshShader = VK_FALSE;
            meshShaderFeature.primitiveFragmentShadingRateMeshShader = VK_FALSE;
            requiredExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }

        if (feature.rayTracing)
        {
            // Request Ray Tracing related features
            auto& asFeature = gpu->requestFeatures<::vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
            asFeature.accelerationStructure = VK_TRUE;
            requiredExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

            auto& rtPipelineFeature = gpu->requestFeatures<::vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
            rtPipelineFeature.rayTracingPipeline = VK_TRUE;
            requiredExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

            auto& rayQueryFeature = gpu->requestFeatures<::vk::PhysicalDeviceRayQueryFeaturesKHR>();
            rayQueryFeature.rayQuery = VK_TRUE;
            requiredExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        }

        // TODO renderdoc unsupported features
        if (!createInfo.enableCapture)
        {
            if (feature.multiDrawIndirect)
            {
                // Request Multi-Draw Features EXT
                auto& multiDrawFeature = gpu->requestFeatures<::vk::PhysicalDeviceMultiDrawFeaturesEXT>();
                multiDrawFeature.multiDraw = VK_TRUE;
                requiredExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
                requiredExtensions.push_back(VK_EXT_MULTI_DRAW_EXTENSION_NAME);
            }

            requiredExtensions.push_back(VK_KHR_PIPELINE_BINARY_EXTENSION_NAME);
            auto& pipelineBinary = gpu->requestFeatures<::vk::PhysicalDevicePipelineBinaryFeaturesKHR>();
            pipelineBinary.pipelineBinaries = VK_TRUE;

            requiredExtensions.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
            auto& descriptorBufferFeatures = gpu->requestFeatures<::vk::PhysicalDeviceDescriptorBufferFeaturesEXT>();
            descriptorBufferFeatures.descriptorBuffer = VK_TRUE;
            descriptorBufferFeatures.descriptorBufferPushDescriptors = VK_TRUE;

            requiredExtensions.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
            auto& maintence5 = gpu->requestFeatures<::vk::PhysicalDeviceMaintenance5FeaturesKHR>();
            maintence5.maintenance5 = VK_TRUE;
        }

        if (feature.bindless)
        {
            auto& descriptorIndexingFeatures = gpu->requestFeatures<::vk::PhysicalDeviceDescriptorIndexingFeatures>();
            descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = ::vk::True;
            descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = ::vk::True;
            descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = ::vk::True;
            descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = ::vk::True;
            descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = ::vk::True;
            descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = ::vk::True;
            descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = ::vk::True;
        }
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

    // verify feature support
    {
        const auto& requiredFeature = createInfo.enabledFeatures;
        const auto& supportFeature = gpu->getProperties().feature;

        if (requiredFeature.rayTracing && !supportFeature.rayTracing)
        {
            CM_LOG_ERR("Ray Tracing feature not supported!");
            APH_ASSERT(false);
        }
        if (requiredFeature.meshShading && !supportFeature.meshShading)
        {
            CM_LOG_ERR("Mesh Shading feature not supported!");
            APH_ASSERT(false);
        }
        if (requiredFeature.multiDrawIndirect && !supportFeature.multiDrawIndirect)
        {
            CM_LOG_ERR("Multi Draw Indrect not supported!");
            APH_ASSERT(false);
        }
        if (requiredFeature.tessellationSupported && !supportFeature.tessellationSupported)
        {
            CM_LOG_ERR("some gpu feature not supported!");
            APH_ASSERT(false);
        }
        if (requiredFeature.samplerAnisotropy && !supportFeature.samplerAnisotropy)
        {
            CM_LOG_ERR("Sampler anisotropy feature not supported!");
            APH_ASSERT(false);
        }
        if (requiredFeature.bindless && !supportFeature.bindless)
        {
            CM_LOG_ERR("Bindless feature not supported!");
            APH_ASSERT(false);
        }
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
    device->m_resourcePool.deviceMemory = new VMADeviceAllocator(createInfo.pInstance, device.get());
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

    if (createInfo.enableCapture)
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
    pDevice->m_resourcePool.shader.clear();

    // TODO
    delete pDevice->m_resourcePool.deviceMemory;
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

Result Device::create(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppLayout,
                      const std::string& debugName)
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
    APH_VR(setDebugObjectName(*ppLayout, debugName));
    return Result::Success;
}

Result Device::create(const ShaderCreateInfo& createInfo, Shader** ppShader, const std::string& debugName)
{
    APH_PROFILER_SCOPE();
    const auto& spv = createInfo.code;

    if (createInfo.compile)
    {
        ::vk::ShaderModuleCreateInfo vkCreateInfo{};
        vkCreateInfo.setCodeSize(spv.size()).setPCode(spv.data());
        auto [result, handle] = getHandle().createShaderModule(vkCreateInfo, vk_allocator());
        *ppShader = m_resourcePool.shader.allocate(createInfo, handle);
        APH_VR(setDebugObjectName(*ppShader, debugName));
    }
    else
    {
        *ppShader = m_resourcePool.shader.allocate(createInfo, VK_NULL_HANDLE);
    }

    return Result::Success;
}

Result Device::create(const ProgramCreateInfo& createInfo, ShaderProgram** ppProgram, const std::string& debugName)
{
    APH_PROFILER_SCOPE();
    bool hasTaskShader = false;
    std::vector<Shader*> shaders{};

    PipelineType pipelineType = {};
    // vs + fs
    if (createInfo.shaders.contains(ShaderStage::VS) && createInfo.shaders.contains(ShaderStage::FS))
    {
        shaders.push_back(createInfo.shaders.at(ShaderStage::VS));
        shaders.push_back(createInfo.shaders.at(ShaderStage::FS));
        pipelineType = PipelineType::Geometry;
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
        pipelineType = PipelineType::Mesh;
    }
    // cs
    else if (createInfo.shaders.contains(ShaderStage::CS))
    {
        shaders.push_back(createInfo.shaders.at(ShaderStage::CS));
        pipelineType = PipelineType::Compute;
    }
    else
    {
        APH_ASSERT(false);
        return { Result::RuntimeError, "Unsupported shader stage combinations." };
    }

    ShaderReflector reflector{ ReflectRequest{ shaders } };
    const auto& combineLayout = reflector.getReflectLayoutMeta();

    // setup descriptor set layouts and pipeline layouts
    SmallVector<DescriptorSetLayout*> setLayouts = {};
    SmallVector<::vk::DescriptorSetLayout> vkSetLayouts;
    VkPipelineLayout pipelineLayout;
    {
        uint32_t numSets = combineLayout.descriptorSetMask.count();
        for (unsigned i = 0; i < numSets; i++)
        {
            DescriptorSetLayoutCreateInfo setLayoutCreateInfo{
                .bindings = reflector.getLayoutBindings(i),
                .poolSizes = reflector.getPoolSizes(i),
            };
            DescriptorSetLayout* layout = {};
            APH_VR(create(setLayoutCreateInfo, &layout));
            setLayouts.push_back(layout);
        }

        if (auto maxBoundDescSets = getPhysicalDevice()->getProperties().maxBoundDescriptorSets;
            numSets > maxBoundDescSets)
        {
            VK_LOG_ERR("Number of sets %u exceeds device limit of %u.", numSets, maxBoundDescSets);
        }

        ::vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        if (numSets)
        {
            vkSetLayouts.reserve(setLayouts.size());
            for (const auto& setLayout : setLayouts)
            {
                vkSetLayouts.push_back(setLayout->getHandle());
            }
            pipelineLayoutCreateInfo.setSetLayouts(vkSetLayouts);
        }

        if (combineLayout.pushConstantRange.stageFlags)
        {
            pipelineLayoutCreateInfo.setPushConstantRanges({ combineLayout.pushConstantRange });
        }

        auto [result, handle] = getHandle().createPipelineLayout(pipelineLayoutCreateInfo, vk_allocator());
        VK_VR(result);
        APH_VR(setDebugObjectName(handle, debugName));
        pipelineLayout = std::move(handle);
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

            if (combineLayout.pushConstantRange.stageFlags)
            {
                soCreateInfo.setPushConstantRanges({ combineLayout.pushConstantRange });
            }
            shaderCreateInfos.push_back(soCreateInfo);
        }

        auto [result, shaderObjects] = getHandle().createShadersEXT(shaderCreateInfos, vk_allocator());
        VK_VR(result);

        for (size_t idx = 0; idx < shaders.size(); ++idx)
        {
            APH_VR(setDebugObjectName(shaderObjects[idx], debugName));
            shaderObjectMaps[shaders[idx]->getStage()] = shaderObjects[idx];
        }
    }

    // TODO
    PipelineLayout layout{ .vertexInput = reflector.getVertexInput(),
                           .pushConstantRange = reflector.getPushConstantRange(),
                           .setLayouts = std::move(setLayouts),
                           .handle = pipelineLayout,
                           .type = pipelineType };

    *ppProgram = m_resourcePool.program.allocate(createInfo, layout, shaderObjectMaps);

    return Result::Success;
}

Result Device::create(const ImageViewCreateInfo& createInfo, ImageView** ppImageView, const std::string& debugName)
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
    APH_VR(setDebugObjectName(*ppImageView, debugName));

    return Result::Success;
}

Result Device::create(const BufferCreateInfo& createInfo, Buffer** ppBuffer, const std::string& debugName)
{
    APH_PROFILER_SCOPE();
    // create buffer
    ::vk::BufferCreateInfo bufferInfo{};
    bufferInfo.setSize(createInfo.size)
        .setUsage(createInfo.usage | ::vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setSharingMode(::vk::SharingMode::eExclusive);
    auto [result, buffer] = getHandle().createBuffer(bufferInfo, vk_allocator());
    *ppBuffer = m_resourcePool.buffer.allocate(createInfo, buffer);
    APH_VR(setDebugObjectName(*ppBuffer, debugName));
    m_resourcePool.deviceMemory->allocate(*ppBuffer);

    return Result::Success;
}

Result Device::create(const ImageCreateInfo& createInfo, Image** ppImage, const std::string& debugName)
{
    APH_PROFILER_SCOPE();
    ::vk::ImageCreateInfo imageCreateInfo{};
    imageCreateInfo.setFlags(createInfo.flags)
        .setImageType(utils::VkCast(createInfo.imageType))
        .setFormat(utils::VkCast(createInfo.format))
        .setMipLevels(createInfo.mipLevels)
        .setArrayLayers(createInfo.arraySize)
        .setSamples(utils::getSampleCountFlags(createInfo.sampleCount))
        .setTiling(::vk::ImageTiling::eOptimal)
        .setUsage(createInfo.usage)
        .setSharingMode(::vk::SharingMode::eExclusive)
        .setInitialLayout(::vk::ImageLayout::eUndefined);

    imageCreateInfo.extent.width = createInfo.extent.width;
    imageCreateInfo.extent.height = createInfo.extent.height;
    imageCreateInfo.extent.depth = createInfo.extent.depth;

    auto [result, image] = getHandle().createImage(imageCreateInfo, vk_allocator());
    VK_VR(result);
    *ppImage = m_resourcePool.image.allocate(this, createInfo, image);
    APH_VR(setDebugObjectName(*ppImage, debugName));
    m_resourcePool.deviceMemory->allocate(*ppImage);

    return Result::Success;
}

void Device::destroy(DescriptorSetLayout* pSetLayout)
{
    APH_PROFILER_SCOPE();
    getHandle().destroyDescriptorSetLayout(pSetLayout->getHandle(), vk_allocator());
    m_resourcePool.setLayout.free(pSetLayout);
}

void Device::destroy(Shader* pShader)
{
    APH_PROFILER_SCOPE();
    getHandle().destroyShaderModule(pShader->getHandle(), vk_allocator());
    m_resourcePool.shader.free(pShader);
}

void Device::destroy(ShaderProgram* pProgram)
{
    APH_PROFILER_SCOPE();

    for (auto* setLayout : pProgram->m_pipelineLayout.setLayouts)
    {
        destroy(setLayout);
    }

    for (auto [_, shaderObject] : pProgram->m_shaderObjects)
    {
        getHandle().destroyShaderEXT(shaderObject, vk_allocator());
    }

    getHandle().destroyPipelineLayout(pProgram->m_pipelineLayout.handle, vk_allocator());
    m_resourcePool.program.free(pProgram);
}

void Device::destroy(Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->free(pBuffer);
    getHandle().destroyBuffer(pBuffer->getHandle(), vk_allocator());
    m_resourcePool.buffer.free(pBuffer);
}

void Device::destroy(Image* pImage)
{
    APH_PROFILER_SCOPE();
    m_resourcePool.deviceMemory->free(pImage);
    getHandle().destroyImage(pImage->getHandle(), vk_allocator());
    m_resourcePool.image.free(pImage);
}

void Device::destroy(ImageView* pImageView)
{
    APH_PROFILER_SCOPE();
    getHandle().destroyImageView(pImageView->getHandle(), vk_allocator());
    m_resourcePool.imageView.free(pImageView);
}

Result Device::create(const SwapChainCreateInfo& createInfo, SwapChain** ppSwapchain, const std::string& debugName)
{
    APH_PROFILER_SCOPE();
    *ppSwapchain = new SwapChain(createInfo, this);
    return Result::Success;
}

void Device::destroy(SwapChain* pSwapchain)
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

Result Device::create(const CommandPoolCreateInfo& createInfo, CommandPool** ppCommandPool,
                      const std::string& debugName)
{
    APH_PROFILER_SCOPE();
    ::vk::CommandPoolCreateInfo vkCreateInfo{};
    vkCreateInfo.setQueueFamilyIndex(createInfo.queue->getFamilyIndex())
        .setFlags(::vk::CommandPoolCreateFlagBits::eTransient);
    auto [res, pool] = getHandle().createCommandPool(vkCreateInfo, vk_allocator());
    VK_VR(res);
    *ppCommandPool = m_resourcePool.commandPool.allocate(this, createInfo, pool);
    return Result::Success;
}

Result Device::create(const SamplerCreateInfo& createInfo, Sampler** ppSampler, const std::string& debugName)
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
    APH_VR(setDebugObjectName(*ppSampler, debugName));
    return Result::Success;
}

void Device::destroy(CommandPool* pPool)
{
    APH_PROFILER_SCOPE();
    pPool->reset(true);
    getHandle().destroyCommandPool(pPool->getHandle(), vk_allocator());
    m_resourcePool.commandPool.free(pPool);
}

void Device::destroy(Sampler* pSampler)
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

void Device::executeCommand(Queue* queue, const CmdRecordCallBack&& func, std::vector<Semaphore*> waitSems,
                            std::vector<Semaphore*> signalSems, Fence* pFence)
{
    APH_PROFILER_SCOPE();

    CommandPool* commandPool = {};
    APH_VR(create({ .queue = queue, .transient = true }, &commandPool));

    CommandBuffer* cmd = nullptr;
    APH_VR(commandPool->allocate(1, &cmd));

    APH_VR(cmd->begin(::vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    func(cmd);
    APH_VR(cmd->end());

    QueueSubmitInfo submitInfo{ .commandBuffers = { cmd },
                                .waitSemaphores = std::move(waitSems),
                                .signalSemaphores = std::move(signalSems) };
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
} // namespace aph::vk
