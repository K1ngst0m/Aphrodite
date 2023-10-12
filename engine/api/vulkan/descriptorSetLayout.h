#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class ImageView;
class Buffer;
class Sampler;
class Image;
class Device;
class DescriptorPool;

struct DescriptorDataRange
{
    uint32_t offset = {};
    uint32_t size = {};
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

class DescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout>
{
public:
    DescriptorSetLayout(Device* device, const VkDescriptorSetLayoutCreateInfo& createInfo,
                        VkDescriptorSetLayout handle);

    Device*                                   getDevice() { return m_pDevice; }
    std::vector<VkDescriptorSetLayoutBinding> getBindings() { return m_bindings; }
    VkDescriptorSet                           allocateSet();
    VkResult                                  freeSet(VkDescriptorSet set);
    VkResult                                  updateSet(const DescriptorUpdateInfo& data, VkDescriptorSet set);

private:
    Device*                                   m_pDevice  = {};
    std::vector<VkDescriptorSetLayoutBinding> m_bindings = {};
    std::unique_ptr<DescriptorPool>           m_pool     = {};
};

}  // namespace aph::vk

#endif  // DESCRIPTORSET_H_
