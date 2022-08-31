#include "vklPipeline.h"

namespace vkl {
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
}
