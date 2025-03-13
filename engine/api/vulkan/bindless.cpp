#include "bindless.h"
#include "device.h"

namespace aph::vk
{
BindlessResource::BindlessResource(ShaderProgram* pProgram, Device* pDevice)
    : m_pDevice(pDevice)
    , m_handleData(pDevice->getPhysicalDevice()->getProperties().uniformBufferAlignment)
{
    // handle descriptor
    {
        auto pSetLayout = pProgram->getSetLayout(BindlessResource::HandleSetIdx);
        m_handleData.pSetLayout = pSetLayout;
        m_handleData.pSet = pSetLayout->allocateSet();
    }

    // update resource
    {
        auto pSetLayout = pProgram->getSetLayout(BindlessResource::ResourceSetIdx);
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
        m_resourceData.addressTableMap =
            std::span{ (uint64_t*)m_pDevice->mapMemory(m_resourceData.pAddressTableBuffer),
                       Resource::AddressTableSize };

        DescriptorUpdateInfo updateInfo{ .binding = eBuffer, .buffers = { m_resourceData.pAddressTableBuffer } };
        APH_VR(m_resourceData.pSet->update(updateInfo));
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
        auto id = HandleId{ static_cast<uint32_t>(m_buffers.size()) };
        APH_ASSERT(id < Resource::AddressTableSize);
        m_buffers.push_back(pBuffer);
        m_bufferIds[pBuffer] = id;
        m_resourceData.addressTableMap[id] = m_pDevice->getDeviceAddress(pBuffer);
    }

    return m_bufferIds.at(pBuffer);
}

BindlessResource::HandleId BindlessResource::updateResource(Image* pImage, ::vk::ImageUsageFlagBits usage)
{
    if (!m_imageIds.contains(pImage))
    {
        APH_ASSERT(m_images.size() < std::numeric_limits<uint32_t>::max());
        auto id = HandleId{ static_cast<uint32_t>(m_images.size()) };
        m_images.push_back(pImage);
        m_imageIds[pImage] = id;

        DescriptorUpdateInfo updateInfo{ .arrayOffset = { id }, .images = { pImage }, .imageUsage = usage };
        switch (usage)
        {
        case ::vk::ImageUsageFlagBits::eSampled:
        {
            updateInfo.binding = eSampledImage;
        }
        break;
        case ::vk::ImageUsageFlagBits::eStorage:
        {
            updateInfo.binding = eStorageImage;
        }
        break;
        default:
            VK_LOG_ERR("Image usage [%s] is invalid in bindless resource.", ::vk::to_string(usage));
            APH_ASSERT(false);
            return HandleId{};
            break;
        }
        m_resourceUpdateInfos.push_back(std::move(updateInfo));
    }

    return m_imageIds.at(pImage);
}

BindlessResource::HandleId BindlessResource::updateResource(Sampler* pSampler)
{
    if (!m_samplerIds.contains(pSampler))
    {
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

    m_handleData.dataBuilder.reset();
}
} // namespace aph::vk
