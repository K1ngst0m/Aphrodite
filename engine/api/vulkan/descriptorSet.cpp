#include "descriptorSet.h"
#include "device.h"

namespace aph::vk
{
DescriptorSetLayout::DescriptorSetLayout(Device* device, CreateInfoType createInfo, HandleType handle,
                                         SmallVector<::vk::DescriptorPoolSize> poolSizes,
                                         SmallVector<::vk::DescriptorSetLayoutBinding> bindings)
    : ResourceHandle(handle, std::move(createInfo))
    , m_pDevice(device)
    , m_poolSizes(std::move(poolSizes))
{
    for (const auto& binding : bindings)
    {
        m_descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
        m_shaderStage |= binding.stageFlags;
        if (binding.descriptorCount == VULKAN_NUM_BINDINGS_BINDLESS_VARYING)
        {
            m_isBindless = true;
        }
        m_bindings[binding.binding] = std::move(binding);
    }

    if (isBindless())
    {
        ::vk::DescriptorPoolCreateFlags bindlessFlags = ::vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
        ::vk::DescriptorPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.setPoolSizes(m_poolSizes)
            .setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | bindlessFlags)
            .setMaxSets(1);

        auto [result, vkPool] = m_pDevice->getHandle().createDescriptorPool(poolCreateInfo, vk_allocator());
        VK_VR(result);
        m_pools.push_back(vkPool);

        ::vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(m_pools[m_currentAllocationPoolIndex]).setSetLayouts({ getHandle() });

        auto [res, handles] = m_pDevice->getHandle().allocateDescriptorSets(allocInfo);
        APH_ASSERT(handles.size() == 1);
        VK_VR(res);

        auto pRetHandle = m_setPools.allocate(this, handles[0]);
        m_descriptorSetCounts.emplace(pRetHandle, m_currentAllocationPoolIndex);
    }
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    // Destroy all allocated descriptor sets.
    for (auto [set, id] : m_descriptorSetCounts)
    {
        m_pDevice->getHandle().freeDescriptorSets(m_pools[id], { set->getHandle() });
        m_setPools.free(set);
    }

    // Destroy all created pools.
    for (auto pool : m_pools)
    {
        m_pDevice->getHandle().destroyDescriptorPool(pool, vk_allocator());
    }
}

DescriptorSet* DescriptorSetLayout::allocateSet()
{
    if (isBindless())
    {
        return m_descriptorSetCounts.cbegin()->first;
    }
    std::lock_guard<std::mutex> lock{ m_mtx };

    // Find the next pool to allocate from.
    while (true)
    {
        // Allocate a new VkDescriptorPool if necessary.
        if (m_pools.size() <= m_currentAllocationPoolIndex)
        {
            ::vk::DescriptorPoolCreateInfo createInfo{};
            createInfo.setPoolSizes(m_poolSizes)
                .setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                .setMaxSets(VULKAN_NUM_SETS_PER_POOL);

            auto [result, handle] = m_pDevice->getHandle().createDescriptorPool(createInfo, vk_allocator());
            VK_VR(result);

            m_pools.push_back(handle);
            m_allocatedSets.push_back(0);
            break;
        }

        if (m_allocatedSets[m_currentAllocationPoolIndex] < VULKAN_NUM_SETS_PER_POOL)
        {
            break;
        }

        ++m_currentAllocationPoolIndex;
    }

    ++m_allocatedSets[m_currentAllocationPoolIndex];

    DescriptorSet* pRetHandle;
    {
        ::vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(m_pools[m_currentAllocationPoolIndex]).setSetLayouts({ getHandle() });

        auto [res, handles] = m_pDevice->getHandle().allocateDescriptorSets(allocInfo);
        APH_ASSERT(handles.size() == 1);
        VK_VR(res);

        pRetHandle = m_setPools.allocate(this, handles[0]);
        m_descriptorSetCounts.emplace(pRetHandle, m_currentAllocationPoolIndex);
    }
    return pRetHandle;
}

Result DescriptorSetLayout::freeSet(DescriptorSet* pSet)
{
    if (isBindless())
    {
        return Result::Success;
    }
    std::lock_guard<std::mutex> lock{ m_mtx };

    // Get the index of the descriptor pool the descriptor set was allocated from.
    auto it = m_descriptorSetCounts.find(pSet);
    if (it == m_descriptorSetCounts.end())
    {
        return { Result::RuntimeError, "descriptor set free error." };
    }

    // Return the descriptor set to the original pool.
    auto poolIndex = it->second;
    m_pDevice->getHandle().freeDescriptorSets(m_pools[poolIndex], { pSet->getHandle() });

    // Remove descriptor set from allocatedDescriptorSets map.
    m_descriptorSetCounts.erase(pSet);

    // Decrement the number of allocated descriptor sets for the pool.
    --m_allocatedSets[poolIndex];

    // Set the next allocation to use this pool index.
    m_currentAllocationPoolIndex = poolIndex;

    // Return success.
    return Result::Success;
}

Result DescriptorSetLayout::updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* set)
{
    auto& bindingInfo = m_bindings[data.binding];
    ::vk::DescriptorType descriptorType = bindingInfo.descriptorType;
    ::vk::WriteDescriptorSet writeInfo{};
    writeInfo.setDstSet(set->getHandle())
        .setDstBinding(data.binding)
        .setDstArrayElement(data.arrayOffset)
        .setDescriptorType(descriptorType);

    std::vector<::vk::DescriptorImageInfo> imageInfos;
    std::vector<::vk::DescriptorBufferInfo> bufferInfos;

    switch (descriptorType)
    {
    case ::vk::DescriptorType::eSampler:
    {
        imageInfos.reserve(data.samplers.size());
        for (auto sampler : data.samplers)
        {
            imageInfos.push_back({ sampler->getHandle() });
        }
        writeInfo.setImageInfo(imageInfos);
    }
    break;

    case ::vk::DescriptorType::eSampledImage:
    {
        imageInfos.reserve(data.images.size());
        for (auto image : data.images)
        {
            imageInfos.push_back({ {}, image->getView()->getHandle(), ::vk::ImageLayout::eShaderReadOnlyOptimal });
        }
        writeInfo.setImageInfo(imageInfos);
    }
    break;

    case ::vk::DescriptorType::eCombinedImageSampler:
    {
        APH_ASSERT(data.images.size() == data.samplers.size());
        imageInfos.reserve(data.images.size());
        for (auto image : data.images)
        {
            imageInfos.push_back({ {}, image->getView()->getHandle(), ::vk::ImageLayout::eShaderReadOnlyOptimal });
        }
        writeInfo.setImageInfo(imageInfos);

        for (std::size_t idx = 0; idx < imageInfos.size(); idx++)
        {
            imageInfos[idx].sampler = data.samplers[idx]->getHandle();
        }
    }
    break;

    case ::vk::DescriptorType::eStorageImage:
    {
        imageInfos.reserve(data.images.size());
        for (auto image : data.images)
        {
            imageInfos.push_back({ {}, image->getView()->getHandle(), ::vk::ImageLayout::eGeneral });
        }
        writeInfo.setImageInfo(imageInfos);
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
        for (auto buffer : data.buffers)
        {
            bufferInfos.push_back({ buffer->getHandle(), 0, ::vk::WholeSize });
        }
        writeInfo.setBufferInfo(bufferInfos);
    }
    break;

    default:
        return { Result::RuntimeError, "Unsupported descriptor type." };
        break;
    }

    m_pDevice->getHandle().updateDescriptorSets({ writeInfo }, {});

    return Result::Success;
}
} // namespace aph::vk
