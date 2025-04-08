#pragma once

#include "allocator/objectPool.h"
#include "api/gpuResource.h"
#include "api/vulkan/shader.h"
#include "common/arrayProxy.h"
#include "common/hash.h"
#include "forward.h"
#include "vkUtils.h"

namespace aph::vk
{
struct DescriptorUpdateInfo
{
    uint32_t binding = {};
    uint32_t arrayOffset = {};

    SmallVector<Image*> images;
    SmallVector<Sampler*> samplers;
    SmallVector<Buffer*> buffers;

    bool operator==(const DescriptorUpdateInfo&) const = default;
};

struct DescriptorSetLayoutCreateInfo
{
    SmallVector<::vk::DescriptorSetLayoutBinding> bindings;
    SmallVector<::vk::DescriptorPoolSize> poolSizes;
};

class DescriptorSetLayout : public ResourceHandle<::vk::DescriptorSetLayout, DescriptorSetLayoutCreateInfo>
{
    friend class ThreadSafeObjectPool<DescriptorSetLayout>;

public:
    DescriptorSet* allocateSet();
    Result freeSet(DescriptorSet* pSet);
    Result updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* pSet);
    ::vk::ShaderStageFlags getShaderStages() const
    {
        return m_shaderStage;
    }
    ::vk::DescriptorType getDescriptorBindingType(uint32_t binding) const
    {
        APH_ASSERT(m_bindings.contains(binding));
        return m_bindings.at(binding).descriptorType;
    }
    bool isBindless() const
    {
        return m_isBindless;
    }

    uint32_t getDynamicUniformCount() const
    {
        if (!m_descriptorTypeCounts.contains(::vk::DescriptorType::eUniformBufferDynamic))
        {
            return 0;
        }
        return m_descriptorTypeCounts.at(::vk::DescriptorType::eUniformBufferDynamic);
    }

private:
    DescriptorSetLayout(Device* device, CreateInfoType createInfo, HandleType handle,
                        SmallVector<::vk::DescriptorPoolSize> poolSizes,
                        SmallVector<::vk::DescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayout();

    Device* m_pDevice = {};

private:
    HashMap<uint32_t, ::vk::DescriptorSetLayoutBinding> m_bindings = {};
    SmallVector<::vk::DescriptorPoolSize> m_poolSizes = {};
    SmallVector<::vk::DescriptorPool> m_pools = {};
    SmallVector<uint32_t> m_allocatedSets = {};
    uint32_t m_currentAllocationPoolIndex = {};
    HashMap<DescriptorSet*, uint32_t> m_descriptorSetCounts = {};
    HashMap<::vk::DescriptorType, uint32_t> m_descriptorTypeCounts = {};
    ::vk::ShaderStageFlags m_shaderStage = {};
    bool m_isBindless = false;
    std::mutex m_mtx = {};
    ThreadSafeObjectPool<DescriptorSet> m_setPools;
};

class DescriptorSet : public ResourceHandle<::vk::DescriptorSet>
{
private:
    friend class ThreadSafeObjectPool<DescriptorSet>;
    DescriptorSet(DescriptorSetLayout* pLayout, HandleType handle)
        : ResourceHandle(handle)
        , m_pLayout(pLayout)
    {
    }

public:
    Result update(const DescriptorUpdateInfo& updateInfo)
    {
        return m_pLayout->updateSet(updateInfo, this);
    }

private:
    DescriptorSetLayout* m_pLayout = {};
};

} // namespace aph::vk
