#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{
class VulkanImageView;
class VulkanBuffer;
class VulkanDevice;
class VulkanDescriptorPool;

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
    size_t count {1};
};

class VulkanDescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout>
{
public:
    VulkanDescriptorSetLayout(VulkanDevice* device, const std::vector<ResourcesBinding>& bindings,
                              VkDescriptorSetLayout handle);

    ~VulkanDescriptorSetLayout();

    VulkanDevice*                 getDevice() { return m_pDevice; }
    std::vector<ResourcesBinding> getBindings() { return m_bindings; }
    VkDescriptorSet               allocateSet(const std::vector<ResourceWrite>& writes = {});
    VkResult                      freeSet(VkDescriptorSet set);

private:
    VulkanDevice*                 m_pDevice   = {};
    std::vector<ResourcesBinding> m_bindings = {};
    VulkanDescriptorPool*         m_pool     = {};
};

}  // namespace aph

#endif  // DESCRIPTORSET_H_
