#include "descriptorSetLayout.h"
#include "device.h"
#include "descriptorPool.h"

namespace aph::vk
{

DescriptorSetLayout::DescriptorSetLayout(Device* device, const std::vector<ResourcesBinding>& bindings,
                                         VkDescriptorSetLayout handle) :
    m_pDevice(device)
{
    m_bindings  = bindings;
    m_pool      = std::make_unique<DescriptorPool>(this);
    getHandle() = handle;
}

VkDescriptorSet DescriptorSetLayout::allocateSet(const std::vector<ResourceWrite>& writes)
{
    auto set = m_pool->allocateSet();
    if(writes.empty())
    {
        return set;
    }

    std::vector<VkWriteDescriptorSet> vkWrites;
    for(uint32_t idx = 0; idx < writes.size(); idx++)
    {
        const auto& write   = writes[idx];
        const auto& binding = m_bindings[idx];

        VkWriteDescriptorSet vkWrite{
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set,
            .dstBinding      = idx,
            .descriptorCount = static_cast<uint32_t>(write.count),
            .descriptorType  = utils::VkCast(binding.resType),
        };

        switch(binding.resType)
        {
        case ResourceType::SAMPLER:
        case ResourceType::SAMPLED_IMAGE:
        case ResourceType::COMBINE_SAMPLER_IMAGE:
        case ResourceType::STORAGE_IMAGE:
        {
            vkWrite.pImageInfo = write.imageInfos;
        }
        break;
        case ResourceType::UNIFORM_BUFFER:
        case ResourceType::STORAGE_BUFFER:
        {
            vkWrite.pBufferInfo = write.bufferInfos;
        }
        break;
        default:
        {
            assert("invalid resource type.");
            return set;
        }
        }

        vkWrites.push_back(vkWrite);
    }
    vkUpdateDescriptorSets(m_pDevice->getHandle(), vkWrites.size(), vkWrites.data(), 0, nullptr);

    return set;
}

VkResult DescriptorSetLayout::freeSet(VkDescriptorSet set)
{
    return m_pool->freeSet(set);
}
}  // namespace aph::vk
