#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include "device.h"

namespace vkl {
class VulkanDescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout> {
public:
    static VulkanDescriptorSetLayout* Create(VkDescriptorSetLayoutCreateInfo * pCreateInfo, VkDescriptorSetLayout handle){
        auto instance = new VulkanDescriptorSetLayout;
        for (auto i = 0; i < pCreateInfo->bindingCount; i++){
            instance->_bindings.push_back(pCreateInfo->pBindings[i]);
        }
        instance->_handle = handle;
        return instance;
    }

    VkDescriptorSetLayoutBinding getBindingInfo(uint32_t index){
        return _bindings[index];
    }

    uint32_t getBindingCount(){
        return _bindings.size();
    }

private:
    std::vector<VkDescriptorSetLayoutBinding> _bindings;
};

class VulkanDescriptorSet : ResourceHandle<VkDescriptorSet>{
public:
private:
};

class VulkanDescriptorPool : ResourceHandle<VkDescriptorPool>{
public:
private:
};

} // namespace vkl

#endif // DESCRIPTORSET_H_
