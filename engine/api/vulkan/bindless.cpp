#include "bindless.h"
#include "device.h"

namespace aph::vk
{
BindlessResource::BindlessResource(DescriptorSetLayout* pSetLayout, Device* pDevice)
    : m_pDevice(pDevice)
    , m_handleResource(pDevice->getPhysicalDevice()->getProperties().uniformBufferAlignment)
{
    // TODO verify if bindless
    m_pSetLayout = pSetLayout;
    APH_ASSERT(m_pSetLayout->isBindless());
    m_pSet = m_pSetLayout->allocateSet();
}

void BindlessResource::buildHandleBuffer(DescriptorSetLayout* pSetLayout)
{
    if (m_handleResource.pBuffer)
    {
        m_pDevice->destroy(m_handleResource.pBuffer);
    }

    // handle gpu buffer
    {
        BufferCreateInfo bufferCreateInfo{ .size = m_handleResource.dataBuilder.getData().size(),
                                           .usage = ::vk::BufferUsageFlagBits::eUniformBuffer,
                                           .domain = BufferDomain::LinkedDeviceHost };
        APH_VR(m_pDevice->create(bufferCreateInfo, &m_handleResource.pBuffer));

        void* pMapped = m_pDevice->mapMemory(m_handleResource.pBuffer);
        APH_ASSERT(pMapped);
        m_handleResource.dataBuilder.writeTo(pMapped);
        m_pDevice->unMapMemory(m_handleResource.pBuffer);
    }

    // handle descriptor
    {
        m_handleResource.pSetLayout = pSetLayout;
        m_handleResource.pSet = pSetLayout->allocateSet();
    }
}

BindlessResource::HandleId BindlessResource::updateResource(Buffer* pBuffer, ::vk::BufferUsageFlagBits2 usage)
{
    if (!m_bufferIds.contains(pBuffer))
    {
        auto id = HandleId{ static_cast<uint32_t>(m_buffers.size()) };
        m_buffers.push_back(pBuffer);
        m_bufferIds[pBuffer] = id;

        DescriptorUpdateInfo updateInfo{
            .arrayOffset = id,
            .range = { 0, ::vk::WholeSize },
            .buffers = { pBuffer },
        };
        switch (usage)
        {
        case ::vk::BufferUsageFlagBits2::eStorageTexelBuffer:
        case ::vk::BufferUsageFlagBits2::eStorageBuffer:
        {
            updateInfo.binding = eStorageBuffer;
        }
        break;
        case ::vk::BufferUsageFlagBits2::eUniformTexelBuffer:
        case ::vk::BufferUsageFlagBits2::eUniformBuffer:
        {
            updateInfo.binding = eUniformBuffer;
        }
        break;
        default:
            VK_LOG_ERR("Buffer usage [%s] is invalid in bindless resource.", ::vk::to_string(usage));
            APH_ASSERT(false);
            return HandleId{};
            break;
        }
        APH_VR(m_pSet->update(updateInfo));
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

        DescriptorUpdateInfo updateInfo{ .arrayOffset = { id }, .images = { pImage } };
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

        DescriptorUpdateInfo updateInfo{ .arrayOffset = { id }, .samplers = { pSampler } };
    }

    return m_samplerIds.at(pSampler);
}

} // namespace aph::vk
