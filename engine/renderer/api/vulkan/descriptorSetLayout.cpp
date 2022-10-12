#include "descriptorSetLayout.h"
#include "descriptorPool.h"

namespace vkl {

// Static mapping between pipeline resource types to Vulkan descriptor types.
static std::unordered_map<PipelineResourceType, VkDescriptorType> ResourceTypeMapping = {
    {PIPELINE_RESOURCE_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLER},
    {PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    {PIPELINE_RESOURCE_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
    {PIPELINE_RESOURCE_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    {PIPELINE_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER},
    {PIPELINE_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
    {PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {PIPELINE_RESOURCE_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
    {PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT},
};

VkResult VulkanDescriptorSetLayout::Create(VulkanDevice *device, const std::vector<PipelineResource> &setResources, VulkanDescriptorSetLayout **ppLayout) {
    // Create a new DescriptorPool instance.
    auto instance     = new VulkanDescriptorSetLayout;
    instance->_device = device;

    // Extract all unique resource types and their counts as well as layout bindings.
    for (auto &resource : setResources) {
        // Skip shader inputs and outputs (NOTE: might be required later at some point).
        // Also other types like subpass inputs, pushconstants, etc. should eventually be handled.
        switch (resource.resourceType) {
        case PIPELINE_RESOURCE_TYPE_INPUT:
        case PIPELINE_RESOURCE_TYPE_OUTPUT:
        case PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER:
            continue;

        default:
            break;
        }

        // Convert from VkPipelineResourceType to VkDescriptorType.
        auto descriptorType = ResourceTypeMapping[resource.resourceType];

        // Populate the Vulkan binding info struct.
        VkDescriptorSetLayoutBinding bindingInfo = {};
        bindingInfo.binding                      = resource.binding;
        bindingInfo.descriptorCount              = resource.arraySize;
        bindingInfo.descriptorType               = ResourceTypeMapping.at(resource.resourceType);
        bindingInfo.stageFlags                   = static_cast<VkShaderStageFlags>(resource.stages);
        instance->_bindings.push_back(bindingInfo);

        // Create a quick lookup table for each binding point.
        instance->_bindingsLookup.emplace(resource.binding, bindingInfo);
    }

    // Create the Vulkan descriptor set layout handle.
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount                    = static_cast<uint32_t>(instance->_bindings.size());
    layoutCreateInfo.pBindings                       = instance->_bindings.data();
    auto result                                      = vkCreateDescriptorSetLayout(instance->_device->getHandle(), &layoutCreateInfo, nullptr, &instance->_handle);
    if (result != VK_SUCCESS) {
        delete instance;
        return result;
    }

    // Allocate a DescriptorPool from the new instance.
    instance->_descriptorPool = new VulkanDescriptorPool(instance);

    // Save handle.
    *ppLayout = instance;

    // Return success.
    return VK_SUCCESS;
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(_device->getHandle(), _handle, nullptr);
    delete _descriptorPool;
};

const VulkanDevice *VulkanDescriptorSetLayout::getDevice() {
    return _device;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanDescriptorSetLayout::getBindings() {
    return _bindings;
}

bool VulkanDescriptorSetLayout::getLayoutBinding(uint32_t bindingIndex, VkDescriptorSetLayoutBinding **pBinding) {
    auto it = _bindingsLookup.find(bindingIndex);
    if (it == _bindingsLookup.end())
        return false;

    *pBinding = &it->second;
    return true;
}

VkResult VulkanDescriptorSetLayout::freeDescriptorSet(VkDescriptorSet descriptorSet) {
    // Free descriptor set handle.
    return _descriptorPool->freeDescriptorSet(descriptorSet);
}

VkDescriptorSet VulkanDescriptorSetLayout::allocateDescriptorSet() {
    // Return new descriptor set allocation.
    return _descriptorPool->allocateDescriptorSet();
}

} // namespace vkl
