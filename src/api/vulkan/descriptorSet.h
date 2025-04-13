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
    uint32_t binding     = {};
    uint32_t arrayOffset = {};

    SmallVector<Image*> images;
    SmallVector<Sampler*> samplers;
    SmallVector<Buffer*> buffers;

    auto operator==(const DescriptorUpdateInfo&) const -> bool = default;
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
    auto allocateSet() -> DescriptorSet*;
    auto freeSet(DescriptorSet* pSet) -> Result;
    auto updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* pSet) -> Result;
    auto getShaderStages() const -> ::vk::ShaderStageFlags;
    auto getDescriptorBindingType(uint32_t binding) const -> ::vk::DescriptorType;
    auto isBindless() const -> bool;
    auto getDynamicUniformCount() const -> uint32_t;

private:
    DescriptorSetLayout(Device* device, CreateInfoType createInfo, HandleType handle,
                        SmallVector<::vk::DescriptorPoolSize> poolSizes,
                        SmallVector<::vk::DescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayout();

    Device* m_pDevice = {};

private:
    HashMap<uint32_t, ::vk::DescriptorSetLayoutBinding> m_bindings = {};
    SmallVector<::vk::DescriptorPoolSize> m_poolSizes              = {};
    SmallVector<::vk::DescriptorPool> m_pools                      = {};
    SmallVector<uint32_t> m_allocatedSets                          = {};
    uint32_t m_currentAllocationPoolIndex                          = {};
    HashMap<DescriptorSet*, uint32_t> m_descriptorSetCounts        = {};
    HashMap<::vk::DescriptorType, uint32_t> m_descriptorTypeCounts = {};
    ::vk::ShaderStageFlags m_shaderStage                           = {};
    bool m_isBindless                                              = false;
    std::mutex m_mtx                                               = {};
    ThreadSafeObjectPool<DescriptorSet> m_setPools;
};

class DescriptorSet : public ResourceHandle<::vk::DescriptorSet>
{
private:
    friend class ThreadSafeObjectPool<DescriptorSet>;
    DescriptorSet(DescriptorSetLayout* pLayout, HandleType handle);

public:
    auto update(const DescriptorUpdateInfo& updateInfo) -> Result;

private:
    DescriptorSetLayout* m_pLayout = {};
};

} // namespace aph::vk
