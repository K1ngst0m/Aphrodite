#include "bindless.h"
#include "device.h"

namespace aph::vk
{
BindlessResource::BindlessResource(Device* pDevice)
    : m_pDevice(pDevice)
    , m_handleData(pDevice->getPhysicalDevice()->getProperties().uniformBufferAlignment)
{

    // handle descriptor
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
            APH_VR(m_pDevice->create(layoutCreateInfo, &pSetLayout));
        }
        m_handleData.pSetLayout = pSetLayout;
        m_handleData.pSet = pSetLayout->allocateSet();
    }

    // update resource
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
                // TODO
                auto descriptorCount = idx == ResourceType::eBuffer ? 1 : VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
                auto descriptorType = descriptorTypeMaps.at(idx);

                binding.setBinding(idx)
                    .setDescriptorCount(descriptorCount)
                    .setStageFlags(::vk::ShaderStageFlagBits::eAll)
                    .setDescriptorType(descriptorType);
                layoutCreateInfo.bindings.push_back(binding);

                ::vk::DescriptorPoolSize poolSize{};
                poolSize.setDescriptorCount(descriptorCount).setType(descriptorType);
                layoutCreateInfo.poolSizes.push_back(poolSize);
            }

            APH_VR(m_pDevice->create(layoutCreateInfo, &pSetLayout, "bindless resource layout"));
        }

        // TODO verify if bindless
        m_resourceData.pSetLayout = pSetLayout;
        APH_ASSERT(m_resourceData.pSetLayout->isBindless());
        m_resourceData.pSet = m_resourceData.pSetLayout->allocateSet();

        // address table buffer
        BufferCreateInfo bufferCreateInfo{
            .size = Resource::AddressTableSize,
            .usage = ::vk::BufferUsageFlagBits::eStorageBuffer,
            .domain = MemoryDomain::Host,
        };
        APH_VR(m_pDevice->create(bufferCreateInfo, &m_resourceData.pAddressTableBuffer, "buffer address table"));
        m_resourceData.addressTableMap = std::span{ (uint64_t*)m_pDevice->mapMemory(m_resourceData.pAddressTableBuffer),
                                                    Resource::AddressTableSize };

        DescriptorUpdateInfo updateInfo{ .binding = eBuffer, .buffers = { m_resourceData.pAddressTableBuffer } };
        APH_VR(m_resourceData.pSet->update(updateInfo));
    }

    // pipeline layout
    {
        // TODO
        ::vk::PipelineLayoutCreateInfo createInfo{};
        auto layouts = { m_resourceData.pSetLayout->getHandle(), m_handleData.pSetLayout->getHandle() };
        createInfo.setSetLayouts(layouts);
        auto [result, handle] = m_pDevice->getHandle().createPipelineLayout(createInfo);
        m_pipelineLayout.setLayouts[eResourceSetIdx] = m_resourceData.pSetLayout;
        m_pipelineLayout.setLayouts[eHandleSetIdx] = m_handleData.pSetLayout;
        m_pipelineLayout.handle = handle;
    }
}

BindlessResource::~BindlessResource()
{
    clear();
}

void BindlessResource::build()
{
    std::lock_guard<std::mutex> lock{ m_mtx };

    // handle gpu buffer
    static uint32_t count = 0;
    if (m_rangeDirty)
    {
        if (m_handleData.pBuffer)
        {
            m_pDevice->destroy(m_handleData.pBuffer);
        }

        BufferCreateInfo bufferCreateInfo{ .size = m_handleData.dataBuilder.getData().size(),
                                           .usage = ::vk::BufferUsageFlagBits::eUniformBuffer,
                                           .domain = MemoryDomain::Host };
        APH_VR(m_pDevice->create(bufferCreateInfo, &m_handleData.pBuffer,
                                 std::format("Bindless Handle Buffer {}", count++)));

        void* pMapped = m_pDevice->mapMemory(m_handleData.pBuffer);
        APH_ASSERT(pMapped);
        m_handleData.dataBuilder.writeTo(pMapped);
        m_pDevice->unMapMemory(m_handleData.pBuffer);

        DescriptorUpdateInfo updateInfo{ .binding = 0, .buffers = { m_handleData.pBuffer } };
        APH_VR(m_handleData.pSet->update(updateInfo));

        m_rangeDirty = false;
    }

    for (const auto& updateInfo : m_resourceUpdateInfos)
    {
        APH_VR(m_resourceData.pSet->update(updateInfo));
    }
    m_resourceUpdateInfos.clear();
}

BindlessResource::HandleId BindlessResource::updateResource(Buffer* pBuffer)
{
    if (!m_bufferIds.contains(pBuffer))
    {
        std::lock_guard<std::mutex> lock{ m_mtx };
        auto id = HandleId{ static_cast<uint32_t>(m_buffers.size()) };
        APH_ASSERT(id < Resource::AddressTableSize);
        m_buffers.push_back(pBuffer);
        m_bufferIds[pBuffer] = id;
        m_resourceData.addressTableMap[id] = m_pDevice->getDeviceAddress(pBuffer);
    }

    return m_bufferIds.at(pBuffer);
}

BindlessResource::HandleId BindlessResource::updateResource(Image* pImage)
{
    if (!m_imageIds.contains(pImage))
    {
        std::lock_guard<std::mutex> lock{ m_mtx };
        APH_ASSERT(m_images.size() < std::numeric_limits<uint32_t>::max());
        auto id = HandleId{ static_cast<uint32_t>(m_images.size()) };
        m_images.push_back(pImage);
        m_imageIds[pImage] = id;

        DescriptorUpdateInfo updateInfo{ .binding = eImage, .arrayOffset = { id }, .images = { pImage } };
        m_resourceUpdateInfos.push_back(std::move(updateInfo));
    }

    return m_imageIds.at(pImage);
}

BindlessResource::HandleId BindlessResource::updateResource(Sampler* pSampler)
{
    if (!m_samplerIds.contains(pSampler))
    {
        std::lock_guard<std::mutex> lock{ m_mtx };
        APH_ASSERT(m_samplers.size() < std::numeric_limits<uint32_t>::max());
        auto id = HandleId{ static_cast<uint32_t>(m_samplers.size()) };
        m_samplers.push_back(pSampler);
        m_samplerIds[pSampler] = id;

        DescriptorUpdateInfo updateInfo{ .binding = eSampler, .arrayOffset = { id }, .samplers = { pSampler } };
        m_resourceUpdateInfos.push_back(std::move(updateInfo));
    }

    return m_samplerIds.at(pSampler);
}
void BindlessResource::clear()
{
    std::lock_guard<std::mutex> lock{ m_mtx };
    if (m_handleData.pBuffer)
    {
        m_pDevice->destroy(m_handleData.pBuffer);
        m_handleData.pBuffer = nullptr;
    }

    if (m_resourceData.pAddressTableBuffer)
    {
        m_pDevice->unMapMemory(m_resourceData.pAddressTableBuffer);
        m_pDevice->destroy(m_resourceData.pAddressTableBuffer);
        m_resourceData.pAddressTableBuffer = nullptr;
    }

    if (m_resourceData.pSetLayout)
    {
        m_pDevice->destroy(m_resourceData.pSetLayout);
        m_resourceData.pSetLayout = nullptr;
    }

    if (m_handleData.pSetLayout)
    {
        m_pDevice->destroy(m_handleData.pSetLayout);
    }

    if (m_pipelineLayout.handle)
    {
        m_pDevice->getHandle().destroy(m_pipelineLayout.handle);
        m_pipelineLayout.handle = ::vk::PipelineLayout{};
    }

    m_handleData.dataBuilder.reset();
}

std::string BindlessResource::generateHandleSource()
{
    std::stringstream ss;
    ss << "import modules.bindless;\n";
    ss << "struct HandleData\n";
    ss << "{\n";

    for (const auto& [name, _] : m_handleNameMap)
    {
        ss << std::format("uint {};\n", name);
    }

    ss << "};\n";

    ss << "[[vk::binding(0, Set::eHandle)]] ConstantBuffer<HandleData> handleData;\n";
    ss << "namespace handle\n";
    ss << "{\n";
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
                        static_assert(false, "unsupported resource type.");
                },
                resource);
        }
        ss << std::format("static bindless::{} {} = bindless::{}(handleData.{});\n", type, name, type, name);
    }
    ss << "}\n";

    return ss.str();
}
} // namespace aph::vk
