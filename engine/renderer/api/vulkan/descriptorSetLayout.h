#ifndef DESCRIPTORSETLAYOUT_H_
#define DESCRIPTORSETLAYOUT_H_

#include "device.h"

namespace vkl {
typedef std::vector<uint32_t> DescriptorSetLayoutHash;

struct MemberInfo {
    BaseType          baseType;
    uint32_t          offset;
    uint32_t          size;
    uint32_t          vecSize;
    uint32_t          columns;
    uint32_t          arraySize;
    char              name[VK_MAX_DESCRIPTION_SIZE];
    const MemberInfo *pNext;
    const MemberInfo *pMembers;
};

struct PipelineResource {
    VkShaderStageFlags   stages;
    PipelineResourceType resourceType;
    BaseType             baseType;
    VkAccessFlags        access;
    uint32_t             set;
    uint32_t             binding;
    uint32_t             location;
    uint32_t             inputAttachmentIndex;
    uint32_t             vecSize;
    uint32_t             columns;
    uint32_t             arraySize;
    uint32_t             offset;
    uint32_t             size;
    char                 name[VK_MAX_DESCRIPTION_SIZE];
    const MemberInfo    *pMembers;
};

class VulkanDescriptorSetLayout : public ResourceHandle<VkDescriptorSetLayout> {
public:
    static VkResult Create(VulkanDevice                        *device,
                           const DescriptorSetLayoutHash       &hash,
                           const std::vector<PipelineResource> &setResources,
                           VulkanDescriptorSetLayout          **pLayout);

    ~VulkanDescriptorSetLayout();

    const VulkanDevice *getDevice();

    std::vector<VkDescriptorSetLayoutBinding> getBindings();

    const DescriptorSetLayoutHash &getHash() const;

    bool getLayoutBinding(uint32_t bindingIndex, VkDescriptorSetLayoutBinding **pBinding);

    VkDescriptorSet allocateDescriptorSet();

    VkResult freeDescriptorSet(VkDescriptorSet descriptorSet);

private:
    VulkanDevice                                              *_device = nullptr;
    DescriptorSetLayoutHash                                    _hash;
    std::vector<VkDescriptorSetLayoutBinding>                  _bindings;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _bindingsLookup;
    VulkanDescriptorPool                                      *_descriptorPool;
};
} // namespace vkl

#endif // DESCRIPTORSETLAYOUT_H_
