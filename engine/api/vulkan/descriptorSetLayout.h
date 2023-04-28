#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{
class ImageView;
class Buffer;
class Device;
class DescriptorPool;

struct ResourcesBinding
{
    ResourceType             resType{};
    std::vector<ShaderStage> stages{};
    size_t                   count{1};
    const VkSampler*         pImmutableSampler{};
};

struct ResourceWrite
{
    VkDescriptorImageInfo*  imageInfos{};
    VkDescriptorBufferInfo* bufferInfos{};
    size_t                  count{1};
};

class DescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout>
{
public:
    DescriptorSetLayout(Device* device, const std::vector<ResourcesBinding>& bindings, VkDescriptorSetLayout handle);

    Device*                       getDevice() { return m_pDevice; }
    std::vector<ResourcesBinding> getBindings() { return m_bindings; }
    VkDescriptorSet               allocateSet(const std::vector<ResourceWrite>& writes = {});
    VkResult                      freeSet(VkDescriptorSet set);

private:
    Device*                         m_pDevice  = {};
    std::vector<ResourcesBinding>   m_bindings = {};
    std::unique_ptr<DescriptorPool> m_pool     = {};
};

}  // namespace aph::vk

#endif  // DESCRIPTORSET_H_
