#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include "threads/spinlock.h"
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
    uint32_t             binding     = {};
    uint32_t             arrayOffset = {};
    DescriptorDataRange* pRanges     = {};

    std::vector<Image*>   images;
    std::vector<Sampler*> samplers;
    std::vector<Buffer*>  buffers;
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
    VolkDeviceTable*                               m_pDeviceTable               = {};
    std::vector<VkDescriptorPoolSize>              m_poolSizes                  = {};
    std::vector<VkDescriptorPool>                  m_pools                      = {};
    std::vector<uint32_t>                          m_allocatedSets              = {};
    uint32_t                                       m_currentAllocationPoolIndex = {};
    std::unordered_map<VkDescriptorSet, uint32_t>  m_allocatedDescriptorSets    = {};
    std::unordered_map<VkDescriptorType, uint32_t> m_descriptorTypeCounts       = {};
    SpinLock                                       m_spinLock                   = {};
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
