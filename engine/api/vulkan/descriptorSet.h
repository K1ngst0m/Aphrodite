#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

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
    uint32_t size   = {};
};

struct DescriptorUpdateInfo
{
    uint32_t            binding     = {};
    uint32_t            arrayOffset = {};
    DescriptorDataRange range       = {};

    std::vector<Image*>   images;
    std::vector<Sampler*> samplers;
    std::vector<Buffer*>  buffers;

    bool operator==(const DescriptorUpdateInfo& other) const noexcept
    {
        if(images.size() != other.images.size() || samplers.size() != other.samplers.size() ||
           buffers.size() != other.buffers.size())
        {
            return false;
        }

        if(binding != other.binding || arrayOffset != other.arrayOffset || range.offset != other.range.offset ||
           range.size != other.range.size)
        {
            return false;
        }

        for(auto idx = 0; idx < images.size(); ++idx)
        {
            if(images[idx] != other.images[idx])
            {
                return false;
            }
        }

        for(auto idx = 0; idx < buffers.size(); ++idx)
        {
            if(buffers[idx] != other.buffers[idx])
            {
                return false;
            }
        }
        for(auto idx = 0; idx < samplers.size(); ++idx)
        {
            if(samplers[idx] != other.samplers[idx])
            {
                return false;
            }
        }

        return true;
    }
};

class DescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout, VkDescriptorSetLayoutCreateInfo>
{
    enum
    {
        DESCRIPTOR_POOL_MAX_NUM_SET = 50,
    };

public:
    DescriptorSetLayout(Device* device, const CreateInfoType& createInfo, HandleType handle);
    ~DescriptorSetLayout();

    Device*                      getDevice() const { return m_pDevice; }
    VkDescriptorSetLayoutBinding getBindings(uint32_t idx) const { return m_bindings[idx]; }
    DescriptorSet*               allocateSet();
    VkResult                     freeSet(const DescriptorSet* pSet);
    VkResult                     updateSet(const DescriptorUpdateInfo& data, const DescriptorSet* pSet);

private:
    Device*                                   m_pDevice  = {};
    std::vector<VkDescriptorSetLayoutBinding> m_bindings = {};

private:
    VolkDeviceTable*                    m_pDeviceTable               = {};
    std::vector<VkDescriptorPoolSize>   m_poolSizes                  = {};
    std::vector<VkDescriptorPool>       m_pools                      = {};
    std::vector<uint32_t>               m_allocatedSets              = {};
    uint32_t                            m_currentAllocationPoolIndex = {};
    HashMap<VkDescriptorSet, uint32_t>  m_allocatedDescriptorSets    = {};
    HashMap<VkDescriptorType, uint32_t> m_descriptorTypeCounts       = {};
    std::mutex                          m_lock                       = {};
};

class DescriptorSet : public ResourceHandle<VkDescriptorSet>
{
public:
    DescriptorSet(DescriptorSetLayout* pLayout, HandleType handle) : ResourceHandle(handle), m_pLayout(pLayout) {}

    void update(const DescriptorUpdateInfo& updateInfo) { m_pLayout->updateSet(updateInfo, this); }
    void free() { m_pLayout->freeSet(this); }

private:
    DescriptorSetLayout* m_pLayout = {};
};

}  // namespace aph::vk

#endif  // DESCRIPTORSET_H_
