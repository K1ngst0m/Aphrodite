#include "descriptorSet.h"
#include "device.h"

namespace aph::vk
{

DescriptorSetLayout::DescriptorSetLayout(Device* device, const CreateInfoType& createInfo, HandleType handle,
                                         const SmallVector<::vk::DescriptorPoolSize>&         poolSizes,
                                         const SmallVector<::vk::DescriptorSetLayoutBinding>& bindings) :
    ResourceHandle(handle, createInfo),
    m_pDevice(device),
    m_poolSizes(poolSizes)
{
    // fill bindings and count of types
    for(std::size_t idx = 0; idx < bindings.size(); idx++)
    {
        auto& binding = bindings[idx];
        m_bindings.push_back(binding);
        m_descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
    }
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    // Destroy all allocated descriptor sets.
    for(auto [set, id] : m_allocatedDescriptorSets)
    {
        m_pDevice->getHandle().freeDescriptorSets(m_pools[id], {set->getHandle()});
    }

    // Destroy all created pools.
    for(auto pool : m_pools)
    {
        m_pDevice->getHandle().destroyDescriptorPool(pool, vk_allocator());
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
            ::vk::DescriptorPoolCreateInfo createInfo{};
            createInfo.setPoolSizes(m_poolSizes)
                .setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                .setMaxSets(DESCRIPTOR_POOL_MAX_NUM_SET);

            ::vk::DescriptorPoolInlineUniformBlockCreateInfo inlineUniformBlockCreateInfo{};
            inlineUniformBlockCreateInfo.setMaxInlineUniformBlockBindings(
                m_descriptorTypeCounts[::vk::DescriptorType::eInlineUniformBlock]);

            if(m_descriptorTypeCounts.contains(::vk::DescriptorType::eInlineUniformBlock))
            {
                createInfo.setPNext(&inlineUniformBlockCreateInfo);
            }

            auto [result, handle] = m_pDevice->getHandle().createDescriptorPool(createInfo, vk_allocator());
            _VR(result);

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
    ::vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.setDescriptorPool(m_pools[m_currentAllocationPoolIndex]).setSetLayouts({getHandle()});

    auto [res, handles] = m_pDevice->getHandle().allocateDescriptorSets(allocInfo);
    APH_ASSERT(handles.size() == 1);
    _VR(res);

    auto pRetHandle = new DescriptorSet(this, handles[0]);
    // Store an internal mapping between the descriptor set handle and it's parent pool.
    // This is used when FreeDescriptorSet is called downstream.
    m_allocatedDescriptorSets.emplace(pRetHandle, m_currentAllocationPoolIndex);

    // Unlock access to internal resources.
    m_lock.unlock();

    // Return descriptor set handle.
    return pRetHandle;
}

Result DescriptorSetLayout::freeSet(DescriptorSet* pSet)
{
    // Safe guard access to internal resources across threads.
    m_lock.lock();

    // Get the index of the descriptor pool the descriptor set was allocated from.
    auto it = m_allocatedDescriptorSets.find(pSet);
    if(it == m_allocatedDescriptorSets.end())
    {
        return {Result::RuntimeError, "descriptor set free error."};
    }

    // Return the descriptor set to the original pool.
    auto poolIndex = it->second;
    m_pDevice->getHandle().freeDescriptorSets(m_pools[poolIndex], {pSet->getHandle()});

    // Remove descriptor set from allocatedDescriptorSets map.
    m_allocatedDescriptorSets.erase(pSet);

    // Decrement the number of allocated descriptor sets for the pool.
    --m_allocatedSets[poolIndex];

    // Set the next allocation to use this pool index.
    m_currentAllocationPoolIndex = poolIndex;

    // Unlock access to internal resources.
    m_lock.unlock();

    // Return success.
    return Result::Success;
}

Result DescriptorSetLayout::updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* set)
{
    APH_ASSERT(data.binding < m_bindings.size());

    auto&                                   bindingInfo    = m_bindings[data.binding];
    ::vk::DescriptorType                    descriptorType = bindingInfo.descriptorType;
    std::vector<::vk::DescriptorImageInfo>  imageInfos;
    std::vector<::vk::DescriptorBufferInfo> bufferInfos;
    ::vk::WriteDescriptorSet                writeInfo{};
    writeInfo.setDstSet(set->getHandle())
        .setDstBinding(data.binding)
        .setDstArrayElement(data.arrayOffset)
        .setDescriptorType(descriptorType);

    switch(descriptorType)
    {
    case ::vk::DescriptorType::eSampler:
    {
        imageInfos.reserve(data.samplers.size());
        for(auto sampler : data.samplers)
        {
            imageInfos.push_back({sampler->getHandle()});
        }
        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();
    }
    break;

    case ::vk::DescriptorType::eSampledImage:
    {
        imageInfos.reserve(data.images.size());
        for(auto image : data.images)
        {
            imageInfos.push_back({{}, image->getView()->getHandle(), ::vk::ImageLayout::eShaderReadOnlyOptimal});
        }
        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();
    }
    break;

    case ::vk::DescriptorType::eCombinedImageSampler:
    {
        APH_ASSERT(data.images.size() == data.samplers.size());
        imageInfos.reserve(data.images.size());
        for(auto image : data.images)
        {
            imageInfos.push_back({{}, image->getView()->getHandle(), ::vk::ImageLayout::eShaderReadOnlyOptimal});
        }
        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();

        for(std::size_t idx = 0; idx < imageInfos.size(); idx++)
        {
            imageInfos[idx].sampler = data.samplers[idx]->getHandle();
        }
    }
    break;

    case ::vk::DescriptorType::eStorageImage:
    {
        imageInfos.reserve(data.images.size());
        for(auto image : data.images)
        {
            imageInfos.push_back({{}, image->getView()->getHandle(), ::vk::ImageLayout::eGeneral});
        }
        writeInfo.pImageInfo      = imageInfos.data();
        writeInfo.descriptorCount = imageInfos.size();
    }
    break;

    case ::vk::DescriptorType::eUniformTexelBuffer:
    case ::vk::DescriptorType::eStorageTexelBuffer:
    case ::vk::DescriptorType::eUniformBuffer:
    case ::vk::DescriptorType::eStorageBuffer:
    case ::vk::DescriptorType::eUniformBufferDynamic:
    case ::vk::DescriptorType::eStorageBufferDynamic:
    {
        bufferInfos.reserve(data.buffers.size());
        for(auto buffer : data.buffers)
        {
            bufferInfos.push_back({buffer->getHandle(), 0, VK_WHOLE_SIZE});
        }
        writeInfo.pBufferInfo     = bufferInfos.data();
        writeInfo.descriptorCount = bufferInfos.size();
    }
    break;

    default:
        return {Result::RuntimeError, "Unsupported descriptor type."};
        break;
    }

    m_pDevice->getHandle().updateDescriptorSets({writeInfo}, {});

    return Result::Success;
}
}  // namespace aph::vk
