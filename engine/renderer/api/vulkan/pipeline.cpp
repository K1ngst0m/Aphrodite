#include "pipeline.h"
#include "renderpass.h"
#include "scene/entity.h"
#include "shader.h"

namespace vkl {
VkVertexInputAttributeDescription VertexInputBuilder::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component) {
    switch (component) {
    case VertexComponent::POSITION:
        return VkVertexInputAttributeDescription({location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)});
    case VertexComponent::NORMAL:
        return VkVertexInputAttributeDescription({location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
    case VertexComponent::UV:
        return VkVertexInputAttributeDescription({location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
    case VertexComponent::COLOR:
        return VkVertexInputAttributeDescription({location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
    case VertexComponent::TANGENT:
        return VkVertexInputAttributeDescription({location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)});
    default:
        return VkVertexInputAttributeDescription({});
    }
}

std::vector<VkVertexInputAttributeDescription> VertexInputBuilder::inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components) {
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t                                       location = 0;
    for (VertexComponent component : components) {
        result.push_back(VertexInputBuilder::inputAttributeDescription(binding, location, component));
        location++;
    }
    return result;
}

VkPipelineVertexInputStateCreateInfo VertexInputBuilder::getPipelineVertexInputState(const std::vector<VertexComponent> &components) {
    _vertexInputBindingDescription      = VkVertexInputBindingDescription({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    _vertexInputAttributeDescriptions   = VertexInputBuilder::inputAttributeDescriptions(0, components);
    _pipelineVertexInputStateCreateInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &_vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexInputBuilder::_vertexInputAttributeDescriptions.size()),
        .pVertexAttributeDescriptions    = VertexInputBuilder::_vertexInputAttributeDescriptions.data(),
    };
    return _pipelineVertexInputStateCreateInfo;
}

VulkanPipeline *VulkanPipeline::CreateGraphicsPipeline(VulkanDevice *pDevice, const PipelineCreateInfo *pCreateInfo, ShaderEffect *pEffect, VulkanRenderPass *pRenderPass, VkPipeline handle) {
    auto *instance    = new VulkanPipeline();
    instance->_handle = handle;
    instance->_effect = pEffect;
    instance->_device = pDevice;
    memcpy(&instance->_createInfo, pCreateInfo, sizeof(PipelineCreateInfo));
    return instance;
}

VulkanPipeline *VulkanPipeline::CreateComputePipeline(VulkanDevice *pDevice, const PipelineCreateInfo *pCreateInfo) {
    auto *instance = new VulkanPipeline();
    return instance;
}

VkPipelineLayout VulkanPipeline::getPipelineLayout() {
    return _effect->getPipelineLayout();
}

VkDescriptorSetLayout *VulkanPipeline::getDescriptorSetLayout(uint32_t idx) {
    return _effect->getDescriptorSetLayout(idx);
}

} // namespace vkl
