#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

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

struct DescriptorDataRange
{
    uint32_t offset = {};
    uint32_t size = {};
};

struct DescriptorUpdateInfo
{
    uint32_t binding = {};
    uint32_t arrayOffset = {};
    DescriptorDataRange range = {};

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
    enum
    {
        DESCRIPTOR_POOL_MAX_NUM_SET = 50,
    };

public:
    Device* getDevice() const
    {
        return m_pDevice;
    }
    DescriptorSet* allocateSet();
    Result freeSet(DescriptorSet* pSet);
    Result updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* pSet);

private:
    DescriptorSetLayout(Device* device, const CreateInfoType& createInfo, HandleType handle,
                        const SmallVector<::vk::DescriptorPoolSize>& poolSizes,
                        const SmallVector<::vk::DescriptorSetLayoutBinding>& bindings);
    ~DescriptorSetLayout();

    Device* m_pDevice = {};
    std::vector<::vk::DescriptorSetLayoutBinding> m_bindings = {};

private:
    SmallVector<::vk::DescriptorPoolSize> m_poolSizes = {};
    SmallVector<::vk::DescriptorPool> m_pools = {};
    SmallVector<uint32_t> m_allocatedSets = {};
    uint32_t m_currentAllocationPoolIndex = {};
    HashMap<DescriptorSet*, uint32_t> m_allocatedDescriptorSets = {};
    HashMap<::vk::DescriptorType, uint32_t> m_descriptorTypeCounts = {};
    std::mutex m_lock = {};
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
    Result free()
    {
        return m_pLayout->freeSet(this);
    }

private:
    DescriptorSetLayout* m_pLayout = {};
};

} // namespace aph::vk

#endif // DESCRIPTORSET_H_
