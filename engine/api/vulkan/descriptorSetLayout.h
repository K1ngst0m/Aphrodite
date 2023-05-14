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

class DescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout>
{
public:
    DescriptorSetLayout(Device* device, const VkDescriptorSetLayoutCreateInfo& createInfo,
                        VkDescriptorSetLayout handle);

    Device*                                   getDevice() { return m_pDevice; }
    std::vector<VkDescriptorSetLayoutBinding> getBindings() { return m_bindings; }
    VkDescriptorSet                           allocateSet();
    VkResult                                  freeSet(VkDescriptorSet set);

private:
    Device*                                   m_pDevice  = {};
    std::vector<VkDescriptorSetLayoutBinding> m_bindings = {};
    std::unique_ptr<DescriptorPool>           m_pool     = {};
};

}  // namespace aph::vk

#endif  // DESCRIPTORSET_H_
