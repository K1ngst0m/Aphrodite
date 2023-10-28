#include "descriptorSet.h"
#include "device.h"

namespace aph::vk
{

DescriptorSetLayout::DescriptorSetLayout(Device* device, const CreateInfoType& createInfo, HandleType handle) :
    ResourceHandle(handle, createInfo),
    m_pDevice(device),
    m_pDeviceTable(device->getDeviceTable())
{
    getHandle() = handle;

    // fill bindings and count of types
    for(std::size_t idx = 0; idx < createInfo.bindingCount; idx++)
    {
        auto& binding = createInfo.pBindings[idx];
        m_bindings.push_back(binding);
        m_descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }

    // calculate pool sizes
    {
        m_poolSizes.resize(m_descriptorTypeCounts.size());
        for(uint32_t index = 0; auto [descriptorType, count] : m_descriptorTypeCounts)
        {
            m_poolSizes[index].type            = descriptorType;
            m_poolSizes[index].descriptorCount = count * DESCRIPTOR_POOL_MAX_NUM_SET;
            ++index;
        }
    }
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    // Destroy all allocated descriptor sets.
    for(auto it : m_allocatedDescriptorSets)
    {
        m_pDeviceTable->vkFreeDescriptorSets(getDevice()->getHandle(), m_pools[it.second], 1, &it.first);
    }

    // Destroy all created pools.
    for(auto pool : m_pools)
    {
        m_pDeviceTable->vkDestroyDescriptorPool(getDevice()->getHandle(), pool, vkAllocator());
    }
}

DescriptorSet* DescriptorSetLayout::allocateSet()
{
    // Safe guard access to internal resources across threads.
    m_lock.lock();

    // Find the next pool to allocate from.
    while(true)
    {
        // Allocate a new VkDescriptorPool if necessary.
        if(m_pools.size() <= m_currentAllocationPoolIndex)
        {
            // Create the Vulkan descriptor pool.
            VkDescriptorPoolCreateInfo createInfo = {
                .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .maxSets       = DESCRIPTOR_POOL_MAX_NUM_SET,
                .poolSizeCount = static_cast<uint32_t>(m_poolSizes.size()),
                .pPoolSizes    = m_poolSizes.data(),
            };
            VkDescriptorPoolInlineUniformBlockCreateInfo descriptorPoolInlineUniformBlockCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO,
                .maxInlineUniformBlockBindings =
                    static_cast<uint32_t>(m_descriptorTypeCounts[VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK]),
            };
            if(m_descriptorTypeCounts.contains(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK))
            {
                createInfo.pNext = &descriptorPoolInlineUniformBlockCreateInfo;
            }
            VkDescriptorPool handle = VK_NULL_HANDLE;
            _VR(m_pDeviceTable->vkCreateDescriptorPool(getDevice()->getHandle(), &createInfo, vkAllocator(), &handle));

            // Add the Vulkan handle to the descriptor pool instance.
            m_pools.push_back(handle);
            m_allocatedSets.push_back(0);
            break;
        }

        if(m_allocatedSets[m_currentAllocationPoolIndex] < DESCRIPTOR_POOL_MAX_NUM_SET)
        {
            break;
        }

        // Increment pool index.
        ++m_currentAllocationPoolIndex;
    }

    // Increment allocated set count for given pool.
    ++m_allocatedSets[m_currentAllocationPoolIndex];

    // Allocate a new descriptor set from the current pool index.
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = m_pools[m_currentAllocationPoolIndex],
        .descriptorSetCount = 1,
        .pSetLayouts        = &getHandle(),
    };
    VkDescriptorSet handle = VK_NULL_HANDLE;
    APH_ASSERT(m_pDeviceTable->vkAllocateDescriptorSets(getDevice()->getHandle(), &allocInfo, &handle));

    // Store an internal mapping between the descriptor set handle and it's parent pool.
    // This is used when FreeDescriptorSet is called downstream.
    m_allocatedDescriptorSets.emplace(handle, m_currentAllocationPoolIndex);

    // Unlock access to internal resources.
    m_lock.unlock();

    // Return descriptor set handle.
    auto pRetHandle = new DescriptorSet(this, handle);
    return pRetHandle;
}

VkResult DescriptorSetLayout::freeSet(const DescriptorSet* pSet)
{
    auto descriptorSet = pSet->getHandle();

    // Safe guard access to internal resources across threads.
    m_lock.lock();

    // Get the index of the descriptor pool the descriptor set was allocated from.
    auto it = m_allocatedDescriptorSets.find(descriptorSet);
    if(it == m_allocatedDescriptorSets.end())
        return VK_INCOMPLETE;

    // Return the descriptor set to the original pool.
    auto poolIndex = it->second;
    m_pDeviceTable->vkFreeDescriptorSets(getDevice()->getHandle(), m_pools[poolIndex], 1, &descriptorSet);

    // Remove descriptor set from allocatedDescriptorSets map.
    m_allocatedDescriptorSets.erase(descriptorSet);

    // Decrement the number of allocated descriptor sets for the pool.
    --m_allocatedSets[poolIndex];

    // Set the next allocation to use this pool index.
    m_currentAllocationPoolIndex = poolIndex;

    // Unlock access to internal resources.
    m_lock.unlock();

    // Return success.
    return VK_SUCCESS;
}

VkResult DescriptorSetLayout::updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* set)
{
    APH_ASSERT(data.binding < m_bindings.size());

    auto&                               bindingInfo    = m_bindings[data.binding];
    VkDescriptorType                    descriptorType = bindingInfo.descriptorType;
    std::vector<VkDescriptorImageInfo>  imageInfos;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    VkWriteDescriptorSet                writeInfo{
                       .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                       .dstSet          = set->getHandle(),
                       .dstBinding      = data.binding,
                       .dstArrayElement = data.arrayOffset,
                       .descriptorType  = descriptorType,
    };
    switch(descriptorType)
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

        for(std::size_t idx = 0; idx < imageInfos.size(); idx++)
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

    m_pDeviceTable->vkUpdateDescriptorSets(m_pDevice->getHandle(), 1, &writeInfo, 0, nullptr);

    return VK_SUCCESS;
}
}  // namespace aph::vk
