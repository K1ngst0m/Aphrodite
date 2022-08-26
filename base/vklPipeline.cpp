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
void PipelineBuilder::setShaders(const ShaderEffect &shaders)
{
    _shaderStages.clear();
    for (const auto &stage : shaders.stages) {
        _shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage.stage, stage.shaderModule));
    }
    _pipelineLayout = shaders.pipelineLayout;
}
void ShaderEffect::build(vkl::Device *device, const std::string &vertCodePath, const std::string &fragCodePath)
{
    {
        std::vector<char> spvCode = vkl::utils::loadSpvFromFile(vertCodePath);
        VkShaderModule shaderModule = device->createShaderModule(spvCode);
        stages.push_back({shaderModule, VK_SHADER_STAGE_VERTEX_BIT});
    }

    {
        std::vector<char> spvCode = vkl::utils::loadSpvFromFile(fragCodePath);
        VkShaderModule shaderModule = device->createShaderModule(spvCode);
        stages.push_back({shaderModule, VK_SHADER_STAGE_FRAGMENT_BIT});
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, constantRanges);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
}
void ShaderEffect::build(vkl::Device *device, const std::string &combinedCodePath)
{
    std::vector<char> spvCode = vkl::utils::loadSpvFromFile(combinedCodePath);
    VkShaderModule shaderModule = device->createShaderModule(spvCode);

    reflectToPipelineLayout(device, spvCode.data(), spvCode.size());
}
void ShaderEffect::reflectToPipelineLayout(vkl::Device *device, const void *spirv_code, size_t spirv_nbytes)
{
    // Generate reflection data for a shader
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_nbytes, spirv_code, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    struct DescriptorSetLayoutData {
        uint32_t setNumber;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    // Enumerate and extract shader's input variables
    uint32_t varCount = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &varCount, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorSet *> sets(varCount);
    result = spvReflectEnumerateDescriptorSets(&module, &varCount, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<DescriptorSetLayoutData> tempSetLayouts(sets.size(), DescriptorSetLayoutData{});
    for (size_t setIdx = 0; setIdx < sets.size(); ++setIdx) {
        const SpvReflectDescriptorSet &currentSetRef = *(sets[setIdx]);
        DescriptorSetLayoutData &layout = tempSetLayouts[setIdx];
        layout.bindings.resize(currentSetRef.binding_count);
        for (uint32_t bindIdx = 0; bindIdx < currentSetRef.binding_count; ++bindIdx) {
            const SpvReflectDescriptorBinding &currentBindingRef = *(currentSetRef.bindings[bindIdx]);
            VkDescriptorSetLayoutBinding &layoutBinding = layout.bindings[bindIdx];
            layoutBinding.binding = currentBindingRef.binding;
            layoutBinding.descriptorType = static_cast<VkDescriptorType>(currentBindingRef.descriptor_type);
            layoutBinding.descriptorCount = 1;
            for (uint32_t dimIdx = 0; dimIdx < currentBindingRef.array.dims_count; ++dimIdx) {
                layoutBinding.descriptorCount *= currentBindingRef.array.dims[dimIdx];
            }
            layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
        }
        layout.setNumber = currentSetRef.set;
        layout.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout.createInfo.bindingCount = currentSetRef.binding_count;
        layout.createInfo.pBindings = layout.bindings.data();
    }

    std::vector<VkPushConstantRange> constantRanges;
    for (size_t blockIdx = 0; blockIdx < module.push_constant_block_count; blockIdx++) {
        VkPushConstantRange range{
            .stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage),
            .offset = module.push_constant_blocks[blockIdx].offset,
            .size = module.push_constant_blocks[blockIdx].size,
        };
    }

    for (size_t idx = 0; idx < tempSetLayouts.size(); idx++) {
        auto &currentSetLayoutsCI = tempSetLayouts[idx];
        vkCreateDescriptorSetLayout(device->logicalDevice, &currentSetLayoutsCI.createInfo, nullptr, &setLayouts[idx]);
    }

    VkPipelineLayoutCreateInfo createInfo = vkl::init::pipelineLayoutCreateInfo(setLayouts, constantRanges);

    vkCreatePipelineLayout(device->logicalDevice, &createInfo, nullptr, &pipelineLayout);

    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
}
void ShaderEffect::pushSetLayout(vkl::Device *device, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
    VkDescriptorSetLayout setLayout;
    VkDescriptorSetLayoutCreateInfo perSceneLayoutInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &perSceneLayoutInfo, nullptr, &setLayout));
    setLayouts.push_back(setLayout);
}
void ShaderEffect::pushConstantRanges(VkPushConstantRange constantRange)
{
    constantRanges.push_back(constantRange);
}
}
