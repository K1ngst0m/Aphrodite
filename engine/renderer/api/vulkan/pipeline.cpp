#include "pipeline.h"
#include "renderpass.h"
#include "scene/mesh.h"
#include "shader.h"

namespace vkl {
namespace {
std::unordered_map<VertexComponent, VkVertexInputAttributeDescription> vertexComponmentMap{
    {VertexComponent::POSITION, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)}},
    {VertexComponent::NORMAL, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}},
    {VertexComponent::UV, {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}},
    {VertexComponent::COLOR, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}},
    {VertexComponent::TANGENT, {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)}},
};
}

VkPipelineVertexInputStateCreateInfo &VertexInputBuilder::getPipelineVertexInputState(const std::vector<VertexComponent> &components) {
    uint32_t location = 0;
    for (VertexComponent component : components) {
        VkVertexInputAttributeDescription desc = vertexComponmentMap[component];
        desc.location                          = location;
        _vertexInputAttributeDescriptions.push_back(desc);
        location++;
    }
    _vertexInputBindingDescriptions     = {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    _pipelineVertexInputStateCreateInfo = vkl::init::pipelineVertexInputStateCreateInfo(_vertexInputBindingDescriptions, _vertexInputAttributeDescriptions);
    return _pipelineVertexInputStateCreateInfo;
}

VulkanPipeline *VulkanPipeline::CreateGraphicsPipeline(VulkanDevice *pDevice, const GraphicsPipelineCreateInfo *pCreateInfo, ShaderEffect *pEffect, VulkanRenderPass *pRenderPass, VkPipeline handle) {
    auto *instance       = new VulkanPipeline();
    instance->_handle    = handle;
    instance->_effect    = pEffect;
    instance->_device    = pDevice;
    instance->_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    return instance;
}

VulkanPipeline *VulkanPipeline::CreateComputePipeline(VulkanDevice *pDevice, ShaderEffect * pEffect, VkPipeline handle) {
    auto *instance = new VulkanPipeline();
    instance->_effect = pEffect;
    instance->_handle = handle;
    instance->_device = pDevice;
    return instance;
}

VkPipelineLayout VulkanPipeline::getPipelineLayout() {
    return _effect->getPipelineLayout();
}

VulkanDescriptorSetLayout *VulkanPipeline::getDescriptorSetLayout(uint32_t idx) {
    return _effect->getDescriptorSetLayout(idx);
}

ShaderEffect *VulkanPipeline::getEffect() {
    return _effect;
}
VkPipelineBindPoint VulkanPipeline::getBindPoint() {
    return _bindPoint;
}
} // namespace vkl
