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

VulkanPipeline *VulkanPipeline::CreateGraphicsPipeline(VulkanDevice *pDevice, const PipelineCreateInfo *pCreateInfo, ShaderEffect *pEffect, VulkanRenderPass *pRenderPass) {
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &pCreateInfo->_viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &pCreateInfo->_scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext                               = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &pCreateInfo->_colorBlendAttachment;

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = nullptr;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const auto [stage, sModule] : pEffect->getStages()) {
        shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage, sModule->getHandle()));
    }
    pipelineInfo.stageCount          = shaderStages.size();
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &pCreateInfo->_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &pCreateInfo->_inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pDynamicState       = &pCreateInfo->_dynamicState;
    pipelineInfo.pRasterizationState = &pCreateInfo->_rasterizer;
    pipelineInfo.pDepthStencilState  = &pCreateInfo->_depthStencil;
    pipelineInfo.pMultisampleState   = &pCreateInfo->_multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.layout              = pEffect->getPipelineLayout();
    pipelineInfo.renderPass          = pRenderPass->getHandle();
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline handle;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(pDevice->getHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle));

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

} // namespace vkl
