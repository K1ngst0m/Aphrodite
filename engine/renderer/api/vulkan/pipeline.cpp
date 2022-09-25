#include "pipeline.h"
#include "scene/entity.h"

namespace vkl {
VkVertexInputBindingDescription VertexInputBuilder::_vertexInputBindingDescription;
std::vector<VkVertexInputAttributeDescription> VertexInputBuilder::_vertexInputAttributeDescriptions;
VkPipelineVertexInputStateCreateInfo VertexInputBuilder::_pipelineVertexInputStateCreateInfo;
VkVertexInputAttributeDescription VertexInputBuilder::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component)
{
    switch (component) {
    case VertexComponent::POSITION:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, pos) });
    case VertexComponent::NORMAL:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, normal) });
    case VertexComponent::UV:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexLayout, uv) });
    case VertexComponent::COLOR:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, color) });
    case VertexComponent::TANGENT:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexLayout, tangent) });
    default:
        return VkVertexInputAttributeDescription({});
    }
}
std::vector<VkVertexInputAttributeDescription> VertexInputBuilder::inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components)
{
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t location = 0;
    for (VertexComponent component : components) {
        result.push_back(VertexInputBuilder::inputAttributeDescription(binding, location, component));
        location++;
    }
    return result;
}
void VertexInputBuilder::setPipelineVertexInputState(const std::vector<VertexComponent> &components)
{
    _vertexInputBindingDescription = VkVertexInputBindingDescription({ 0, sizeof(VertexLayout), VK_VERTEX_INPUT_RATE_VERTEX });
    _vertexInputAttributeDescriptions = VertexInputBuilder::inputAttributeDescriptions(0, components);
    _pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &VertexInputBuilder::_vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexInputBuilder::_vertexInputAttributeDescriptions.size()),
        .pVertexAttributeDescriptions = VertexInputBuilder::_vertexInputAttributeDescriptions.data(),
    };
}
VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass)
{
    //make viewport state from our stored viewport and scissor.
    //at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &_scissor;

    //setup dummy color blending. We aren't using transparent objects yet
    //the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;

    //build the actual pipeline
    //we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.stageCount = _shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = &_dynamicState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pDepthStencilState = &_depthStencil;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline));

    return newPipeline;
}
void PipelineBuilder::setShaders(ShaderEffect *shaders)
{
    _shaderStages.clear();
    for (const auto &stage : shaders->stages) {
        _shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage.stage, stage.shaderModule->module));
    }
    _pipelineLayout = shaders->builtLayout;
}
void ShaderEffect::buildPipelineLayout(VkDevice device)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, constantRanges);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &builtLayout));
}
void ShaderEffect::pushSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
    VkDescriptorSetLayout setLayout;
    VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &perSceneLayoutInfo, nullptr, &setLayout));
    setLayouts.push_back(setLayout);
}
void ShaderEffect::pushConstantRanges(VkPushConstantRange constantRange)
{
    constantRanges.push_back(constantRange);
}
void ShaderEffect::pushShaderStages(ShaderModule *module, VkShaderStageFlagBits stageBits)
{
    stages.push_back({ module, stageBits });
}
void ShaderPass::build(VkDevice device, VkRenderPass renderPass, PipelineBuilder &builder, vkl::ShaderEffect *shaderEffect)
{
    effect = shaderEffect;
    layout = shaderEffect->builtLayout;

    PipelineBuilder pipbuilder = builder;

    pipbuilder.setShaders(shaderEffect);

    builtPipeline = pipbuilder.buildPipeline(device, renderPass);
}
void PipelineBuilder::resetToDefault(VkExtent2D extent) {
    VertexInputBuilder::setPipelineVertexInputState({VertexComponent::POSITION, VertexComponent::NORMAL,
                                                          VertexComponent::UV, VertexComponent::COLOR, VertexComponent::TANGENT});
    _vertexInputInfo = vkl::VertexInputBuilder::_pipelineVertexInputStateCreateInfo;
    _inputAssembly   = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    _viewport        = vkl::init::viewport(extent);
    _scissor         = vkl::init::rect2D(extent);

    _dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    _dynamicState  = vkl::init::pipelineDynamicStateCreateInfo(_dynamicStages.data(), static_cast<uint32_t>(_dynamicStages.size()));

    _rasterizer           = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    _multisampling        = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    _colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    _depthStencil         = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
}
} // namespace vkl
