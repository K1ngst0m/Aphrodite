#pragma once

#include "api/vulkan/shader.h"
#include "common/hash.h"
#include "vkUtils.h"

namespace aph::vk
{
class ImageView;
class Buffer;
class Sampler;
class Image;
class Device;
class DescriptorSet;

struct DescriptorUpdateInfo
{
    uint32_t binding = {};
    uint32_t arrayOffset = {};
    Range range = {};

    std::vector<Image*> images;
    std::vector<Sampler*> samplers;
    std::vector<Buffer*> buffers;

    bool operator==(const DescriptorUpdateInfo& other) const noexcept
    {
        if (images.size() != other.images.size() || samplers.size() != other.samplers.size() ||
            buffers.size() != other.buffers.size())
        {
            return false;
        }

        if (binding != other.binding || arrayOffset != other.arrayOffset || range.offset != other.range.offset ||
            range.size != other.range.size)
        {
            return false;
        }

        for (auto idx = 0; idx < images.size(); ++idx)
        {
            if (images[idx] != other.images[idx])
            {
                return false;
            }
        }

        for (auto idx = 0; idx < buffers.size(); ++idx)
        {
            if (buffers[idx] != other.buffers[idx])
            {
                return false;
            }
        }
        for (auto idx = 0; idx < samplers.size(); ++idx)
        {
            if (samplers[idx] != other.samplers[idx])
            {
                return false;
            }
        }

        return true;
    }
};

struct DescriptorSetLayoutCreateInfo
{
    SmallVector<::vk::DescriptorSetLayoutBinding> bindings;
    SmallVector<::vk::DescriptorPoolSize> poolSizes;
};

class DescriptorSetLayout : public ResourceHandle<::vk::DescriptorSetLayout, DescriptorSetLayoutCreateInfo>
{
    friend class ObjectPool<DescriptorSetLayout>;

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
        return m_bindings[binding].descriptorType;
    }

private:
    DescriptorSetLayout(Device* device, CreateInfoType createInfo, HandleType handle,
                        SmallVector<::vk::DescriptorPoolSize> poolSizes,
                        SmallVector<::vk::DescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayout();

    Device* m_pDevice = {};

private:
    SmallVector<::vk::DescriptorSetLayoutBinding> m_bindings = {};
    SmallVector<::vk::DescriptorPoolSize> m_poolSizes = {};
    SmallVector<::vk::DescriptorPool> m_pools = {};
    SmallVector<uint32_t> m_allocatedSets = {};
    uint32_t m_currentAllocationPoolIndex = {};
    HashMap<DescriptorSet*, uint32_t> m_descriptorSetCounts = {};
    HashMap<::vk::DescriptorType, uint32_t> m_descriptorTypeCounts = {};
    ::vk::ShaderStageFlags m_shaderStage = {};
    std::mutex m_mtx = {};
};

class DescriptorSet : public ResourceHandle<::vk::DescriptorSet>
{
public:
    DescriptorSet(DescriptorSetLayout* pLayout, HandleType handle)
        : ResourceHandle(handle)
        , m_pLayout(pLayout)
    {
    }

    Result update(const DescriptorUpdateInfo& updateInfo)
    {
        return m_pLayout->updateSet(updateInfo, this);
    }

private:
    DescriptorSetLayout* m_pLayout = {};
};

} // namespace aph::vk
