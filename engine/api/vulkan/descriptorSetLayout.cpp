#include "descriptorSetLayout.h"
#include "device.h"
#include "descriptorPool.h"

namespace aph::vk
{

DescriptorSetLayout::DescriptorSetLayout(Device* device, const VkDescriptorSetLayoutCreateInfo& createInfo,
                                         VkDescriptorSetLayout handle) :
    m_pDevice(device)
{
    for(uint32_t idx = 0; idx < createInfo.bindingCount; idx++)
    {
        m_bindings.push_back(createInfo.pBindings[idx]);
    }
    m_pool      = std::make_unique<DescriptorPool>(this);
    getHandle() = handle;
}

VkDescriptorSet DescriptorSetLayout::allocateSet()
{
    return m_pool->allocateSet();
}

VkResult DescriptorSetLayout::freeSet(VkDescriptorSet set)
{
    return m_pool->freeSet(set);
}

VkResult DescriptorSetLayout::updateSet(const DescriptorUpdateInfo& data, VkDescriptorSet set)
{
    APH_ASSERT(data.binding < m_bindings.size());

    auto&                               bindingInfo = m_bindings[data.binding];
    VkDescriptorType                    type        = bindingInfo.descriptorType;
    std::vector<VkDescriptorImageInfo>  imageInfos;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    VkWriteDescriptorSet                writeInfo{
                       .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                       .dstSet          = set,
                       .dstBinding      = data.binding,
                       .dstArrayElement = data.arrayOffset,
                       .descriptorType  = type,
    };
    switch(type)
    {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
    {
        imageInfos.reserve(data.samplers.size());
        for(auto sampler : data.samplers)
        {
            imageInfos.push_back({.sampler = sampler->getHandle()});
        }
        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();
    }
    break;
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    {
        imageInfos.reserve(data.images.size());
        for(auto image : data.images)
        {
            imageInfos.push_back(
                {.imageView = image->getView()->getHandle(), .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();
    }
    break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
    {
        APH_ASSERT(data.images.size() == data.samplers.size());
        imageInfos.reserve(data.images.size());
        for(auto image : data.images)
        {
            imageInfos.push_back(
                {.imageView = image->getView()->getHandle(), .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }
        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();

        for(uint32_t idx = 0; idx < imageInfos.size(); idx++)
        {
            imageInfos[idx].sampler = data.samplers[idx]->getHandle();
        }
    }
    break;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    {
        imageInfos.reserve(data.images.size());
        for(auto image : data.images)
        {
            imageInfos.push_back({.imageView = image->getView()->getHandle(), .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
        }

        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();
    }
    break;
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    {
        bufferInfos.reserve(data.buffers.size());
        for(auto buffer : data.buffers)
        {
            bufferInfos.push_back({.buffer = buffer->getHandle(), .offset = 0, .range = VK_WHOLE_SIZE});
        }

        writeInfo.pBufferInfo     = bufferInfos.data();
        writeInfo.descriptorCount = bufferInfos.size();
    }
    break;
    default:
        VK_LOG_ERR("Unsupported descriptor type.");
        return VK_ERROR_FEATURE_NOT_PRESENT;
        break;
    }

    vkUpdateDescriptorSets(m_pDevice->getHandle(), 1, &writeInfo, 0, nullptr);

    return VK_SUCCESS;
}
}  // namespace aph::vk
