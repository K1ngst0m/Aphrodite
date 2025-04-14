#include "bindless.h"
#include "device.h"

namespace aph::vk
{
BindlessResource::BindlessResource(Device* pDevice)
    : m_pDevice(pDevice)
    , m_handleData(pDevice->getPhysicalDevice()->getProperties().uniformBufferAlignment)
{
    APH_PROFILER_SCOPE();
    // Initialize handle descriptor set layout and allocate descriptor set
    {
        DescriptorSetLayout* pSetLayout = {};
        {
            DescriptorSetLayoutCreateInfo layoutCreateInfo{};
            ::vk::DescriptorSetLayoutBinding binding{};
            binding.setStageFlags(::vk::ShaderStageFlagBits::eAll)
                .setDescriptorType(::vk::DescriptorType::eUniformBufferDynamic)
                .setBinding(0)
                .setDescriptorCount(1);
            layoutCreateInfo.bindings.push_back(binding);
            ::vk::DescriptorPoolSize poolSize{};
            poolSize.setDescriptorCount(1).setType(::vk::DescriptorType::eUniformBuffer);
            layoutCreateInfo.poolSizes.push_back(poolSize);
            auto result = m_pDevice->create(layoutCreateInfo, "bindless resource layout");
            APH_VERIFY_RESULT(result);
            pSetLayout = result.value();
        }
        m_handleData.pSetLayout = pSetLayout;
        m_handleData.pSet       = pSetLayout->allocateSet();
    }

    // Initialize resource descriptor set layout and allocate descriptor set
    {
        DescriptorSetLayout* pSetLayout = {};
        {
            constexpr std::array descriptorTypeMaps{ ::vk::DescriptorType::eSampledImage,
                                                     ::vk::DescriptorType::eStorageBuffer,
                                                     ::vk::DescriptorType::eSampler };

            DescriptorSetLayoutCreateInfo layoutCreateInfo{};

            for (uint32_t idx = 0; idx < eResourceTypeCount; ++idx)
            {
                ::vk::DescriptorSetLayoutBinding binding{};
                // Use a single descriptor for buffer address table
                auto descriptorCount = idx == ResourceType::eBuffer ? 1 : VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
                auto descriptorType  = descriptorTypeMaps.at(idx);

                binding.setBinding(idx)
                    .setDescriptorCount(descriptorCount)
                    .setStageFlags(::vk::ShaderStageFlagBits::eAll)
                    .setDescriptorType(descriptorType);
                layoutCreateInfo.bindings.push_back(binding);

                ::vk::DescriptorPoolSize poolSize{};
                poolSize.setDescriptorCount(descriptorCount).setType(descriptorType);
                layoutCreateInfo.poolSizes.push_back(poolSize);
            }

            auto result = m_pDevice->create(layoutCreateInfo, "bindless resource layout");
            APH_VERIFY_RESULT(result);
            pSetLayout = result.value();
        }

        m_resourceData.pSetLayout = pSetLayout;
        APH_ASSERT(m_resourceData.pSetLayout->isBindless());
        m_resourceData.pSet = m_resourceData.pSetLayout->allocateSet();

        // Create and bind buffer address table
        {
            BufferCreateInfo bufferCreateInfo{
                .size   = Resource::AddressTableSize,
                .usage  = BufferUsage::Storage,
                .domain = MemoryDomain::Host,
            };
            auto result = m_pDevice->create(bufferCreateInfo, "buffer address table");
            APH_VERIFY_RESULT(result);
            m_resourceData.pAddressTableBuffer = result.value();
            m_resourceData.addressTableMap =
                std::span{ (uint64_t*)m_pDevice->mapMemory(m_resourceData.pAddressTableBuffer),
                           Resource::AddressTableSize };
            DescriptorUpdateInfo updateInfo{ .binding = eBuffer, .buffers = { m_resourceData.pAddressTableBuffer } };
            APH_VERIFY_RESULT(m_resourceData.pSet->update(updateInfo));
        }
    }

    // Create pipeline layout resource
    {
        PipelineLayoutCreateInfo createInfo{};
        createInfo.setLayouts.resize(eUpperBound);
        createInfo.setLayouts[eResourceSetIdx] = m_resourceData.pSetLayout;
        createInfo.setLayouts[eHandleSetIdx]   = m_handleData.pSetLayout;
        auto result                            = m_pDevice->create(createInfo);
        APH_VERIFY_RESULT(result);
        m_pipelineLayout = result.value();
    }
}

BindlessResource::~BindlessResource()
{
    APH_PROFILER_SCOPE();
    clear();
}

auto BindlessResource::build() -> void
{
    APH_PROFILER_SCOPE();

    // Update handle buffer if data has changed
    {
        std::lock_guard<std::mutex> lock{ m_handleMtx };
        static uint32_t count = 0;
        if (m_rangeDirty.load(std::memory_order_acquire))
        {
            if (m_handleData.pBuffer)
            {
                m_pDevice->destroy(m_handleData.pBuffer);
            }

            // Update handle buffer
            {
                BufferCreateInfo bufferCreateInfo{ .size   = m_handleData.dataBuilder.getData().size(),
                                                   .usage  = BufferUsage::Uniform,
                                                   .domain = MemoryDomain::Host };
                auto result = m_pDevice->create(bufferCreateInfo, std::format("Bindless Handle Buffer {}", count++));
                APH_VERIFY_RESULT(result);
                m_handleData.pBuffer = result.value();
                void* pMapped        = m_pDevice->mapMemory(m_handleData.pBuffer);
                APH_ASSERT(pMapped);
                m_handleData.dataBuilder.writeTo(pMapped);
                m_pDevice->unMapMemory(m_handleData.pBuffer);
                DescriptorUpdateInfo updateInfo{ .binding = 0, .buffers = { m_handleData.pBuffer } };
                APH_VERIFY_RESULT(m_handleData.pSet->update(updateInfo));
            }

            m_rangeDirty.store(false, std::memory_order_release);
        }
    }

    // Apply all pending descriptor updates to the resource set
    SmallVector<DescriptorUpdateInfo> updateInfos;
    {
        std::lock_guard<std::mutex> lock{ m_updateInfoMtx };
        updateInfos = std::move(m_resourceUpdateInfos);
    }
    for (const auto& updateInfo : updateInfos)
    {
        APH_VERIFY_RESULT(m_resourceData.pSet->update(updateInfo));
    }
}

auto BindlessResource::updateResource(RType resource, std::string name) -> uint32_t
{
    APH_PROFILER_SCOPE();

    // Register resource name in the handle map
    {
        std::unique_lock<std::shared_mutex> lock{ m_nameMtx };
        m_handleNameMap[name] = resource;
    }

    uint32_t offset = 0;

    std::visit(
        [this, &offset](auto&& arg)
        {
            // Register the specific resource type and add handle ID to the buffer
            HandleId id = updateResource(arg);
            {
                std::lock_guard<std::mutex> handleLock{ m_handleMtx };
                offset = m_handleData.dataBuilder.addRange(id);
                m_rangeDirty.store(true, std::memory_order_release);
            }
        },
        resource);

    return offset;
}

auto BindlessResource::updateResource(Buffer* pBuffer) -> HandleId
{
    std::unique_lock<std::shared_mutex> lock{ m_resourceMapsMtx };
    if (!m_bufferIds.contains(pBuffer))
    {
        // Register new buffer and store its device address
        auto id = HandleId{ static_cast<uint32_t>(m_buffers.size()) };
        APH_ASSERT(id < Resource::AddressTableSize);
        m_buffers.push_back(pBuffer);
        m_bufferIds[pBuffer]               = id;
        m_resourceData.addressTableMap[id] = m_pDevice->getDeviceAddress(pBuffer);
    }

    return m_bufferIds.at(pBuffer);
}

auto BindlessResource::updateResource(Image* pImage) -> HandleId
{
    std::unique_lock<std::shared_mutex> lock{ m_resourceMapsMtx };
    if (!m_imageIds.contains(pImage))
    {
        // Register new image and queue descriptor update
        APH_ASSERT(m_images.size() < std::numeric_limits<uint32_t>::max());
        auto id = HandleId{ static_cast<uint32_t>(m_images.size()) };
        m_images.push_back(pImage);
        m_imageIds[pImage] = id;

        DescriptorUpdateInfo updateInfo{ .binding = eImage, .arrayOffset = { id }, .images = { pImage } };

        // Queue descriptor update for next build() call
        {
            std::lock_guard<std::mutex> updateLock{ m_updateInfoMtx };
            m_resourceUpdateInfos.push_back(std::move(updateInfo));
        }
    }

    return m_imageIds.at(pImage);
}

auto BindlessResource::updateResource(Sampler* pSampler) -> HandleId
{
    std::unique_lock<std::shared_mutex> lock{ m_resourceMapsMtx };
    if (!m_samplerIds.contains(pSampler))
    {
        // Register new sampler and queue descriptor update
        APH_ASSERT(m_samplers.size() < std::numeric_limits<uint32_t>::max());
        auto id = HandleId{ static_cast<uint32_t>(m_samplers.size()) };
        m_samplers.push_back(pSampler);
        m_samplerIds[pSampler] = id;

        DescriptorUpdateInfo updateInfo{ .binding = eSampler, .arrayOffset = { id }, .samplers = { pSampler } };

        // Queue descriptor update for next build() call
        {
            std::lock_guard<std::mutex> updateLock{ m_updateInfoMtx };
            m_resourceUpdateInfos.push_back(std::move(updateInfo));
        }
    }

    return m_samplerIds.at(pSampler);
}

auto BindlessResource::clear() -> void
{
    APH_PROFILER_SCOPE();
    // Store resources that need to be destroyed
    Buffer* handleBuffer           = nullptr;
    Buffer* addressTableBuffer     = nullptr;
    PipelineLayout* pipelineLayout = nullptr;

    {
        std::lock_guard<std::mutex> handleLock{ m_handleMtx };
        std::unique_lock<std::shared_mutex> nameLock{ m_nameMtx };
        std::unique_lock<std::shared_mutex> resourceLock{ m_resourceMapsMtx };
        std::lock_guard<std::mutex> updateLock{ m_updateInfoMtx };

        // Store resources for destruction outside the lock
        handleBuffer       = m_handleData.pBuffer;
        addressTableBuffer = m_resourceData.pAddressTableBuffer;
        pipelineLayout     = m_pipelineLayout;

        // Reset member variables to null state
        m_handleData.pBuffer               = nullptr;
        m_resourceData.pAddressTableBuffer = nullptr;
        m_resourceData.pSetLayout          = nullptr;
        m_handleData.pSetLayout            = nullptr;
        m_pipelineLayout                   = nullptr;

        // Clear all collections and reset state
        m_images.clear();
        m_buffers.clear();
        m_samplers.clear();
        m_imageIds.clear();
        m_bufferIds.clear();
        m_samplerIds.clear();
        m_handleNameMap.clear();
        m_resourceUpdateInfos.clear();
        m_handleData.dataBuilder.reset();
        m_rangeDirty.store(false, std::memory_order_relaxed);
    }

    if (handleBuffer)
    {
        m_pDevice->destroy(handleBuffer);
    }

    if (addressTableBuffer)
    {
        m_pDevice->unMapMemory(addressTableBuffer);
        m_pDevice->destroy(addressTableBuffer);
    }

    if (pipelineLayout)
    {
        m_pDevice->destroy(pipelineLayout);
    }
}

auto BindlessResource::generateHandleSource() const -> std::string
{
    APH_PROFILER_SCOPE();

    std::shared_lock<std::shared_mutex> nameLock{ m_nameMtx };

    // Build Slang source code for bindless resource access
    std::stringstream ss;
    ss << "import modules.bindless;\n";
    ss << "struct HandleData\n";
    ss << "{\n";

    // Generate uint handle fields for each named resource
    for (const auto& [name, _] : m_handleNameMap)
    {
        ss << std::format("uint {};\n", name);
    }

    ss << "};\n";

    // Create constant buffer binding for the handle data
    ss << "[[vk::binding(0, Set::eHandle)]] ConstantBuffer<HandleData> handleData;\n";
    ss << "namespace handle\n";
    ss << "{\n";

    // Create typed accessors
    for (const auto& [name, resource] : m_handleNameMap)
    {
        std::string type;
        {
            std::visit(
                [&type](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, Image*>)
                        type = "Texture";
                    else if constexpr (std::is_same_v<T, Buffer*>)
                        type = "Buffer";
                    else if constexpr (std::is_same_v<T, Sampler*>)
                        type = "Sampler2D";
                    else
                        static_assert(dependent_false_v<T>, "unsupported resource type.");
                },
                resource);
        }
        ss << std::format("static bindless::{} {} = bindless::{}(handleData.{});\n", type, name, type, name);
    }
    ss << "}\n";

    return ss.str();
}

auto BindlessResource::getResourceLayout() const noexcept -> DescriptorSetLayout*
{
    return m_resourceData.pSetLayout;
}

auto BindlessResource::getHandleLayout() const noexcept -> DescriptorSetLayout*
{
    return m_handleData.pSetLayout;
}

auto BindlessResource::getResourceSet() const noexcept -> DescriptorSet*
{
    APH_ASSERT(m_resourceData.pSet);
    return m_resourceData.pSet;
}

auto BindlessResource::getHandleSet() const noexcept -> DescriptorSet*
{
    APH_ASSERT(m_handleData.pSet);
    return m_handleData.pSet;
}

auto BindlessResource::getPipelineLayout() const noexcept -> PipelineLayout*
{
    return m_pipelineLayout;
}
} // namespace aph::vk
