#include "pipeline.h"
#include "scene/entity.h"
#include "shader.h"
#include "vulkan/vulkan_core.h"

namespace vkl {
VkVertexInputBindingDescription                VertexInputBuilder::_vertexInputBindingDescription;
std::vector<VkVertexInputAttributeDescription> VertexInputBuilder::_vertexInputAttributeDescriptions;
VkPipelineVertexInputStateCreateInfo           VertexInputBuilder::_pipelineVertexInputStateCreateInfo;
VkVertexInputAttributeDescription              VertexInputBuilder::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component) {
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
void VertexInputBuilder::setPipelineVertexInputState(const std::vector<VertexComponent> &components) {
    _vertexInputBindingDescription      = VkVertexInputBindingDescription({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    _vertexInputAttributeDescriptions   = VertexInputBuilder::inputAttributeDescriptions(0, components);
    _pipelineVertexInputStateCreateInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &VertexInputBuilder::_vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexInputBuilder::_vertexInputAttributeDescriptions.size()),
        .pVertexAttributeDescriptions    = VertexInputBuilder::_vertexInputAttributeDescriptions.data(),
    };
}
VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass) {
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &_createInfo._viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &_createInfo._scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext                               = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &_createInfo._colorBlendAttachment;

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = nullptr;

    pipelineInfo.stageCount          = _createInfo._shaderStages.size();
    pipelineInfo.pStages             = _createInfo._shaderStages.data();
    pipelineInfo.pVertexInputState   = &_createInfo._vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_createInfo._inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pDynamicState       = &_createInfo._dynamicState;
    pipelineInfo.pRasterizationState = &_createInfo._rasterizer;
    pipelineInfo.pDepthStencilState  = &_createInfo._depthStencil;
    pipelineInfo.pMultisampleState   = &_createInfo._multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.layout              = _createInfo._pipelineLayout;
    pipelineInfo.renderPass          = pass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
    VkPipeline newPipeline;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline));

    return newPipeline;
}
void PipelineBuilder::setShaders(VulkanPipelineLayout *shaders) {
    _createInfo._shaderStages.clear();
    for (const auto &stage : shaders->stages) {
        _createInfo._shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage.stage, stage.shaderModule->module));
    }
    _createInfo._pipelineLayout = shaders->builtLayout;
}
void PipelineBuilder::reset(VkExtent2D extent) {
    VertexInputBuilder::setPipelineVertexInputState({VertexComponent::POSITION,
                                                     VertexComponent::NORMAL,
                                                     VertexComponent::UV,
                                                     VertexComponent::COLOR,
                                                     VertexComponent::TANGENT});

    _createInfo._vertexInputInfo = vkl::VertexInputBuilder::_pipelineVertexInputStateCreateInfo;
    _createInfo._inputAssembly   = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    _createInfo._viewport        = vkl::init::viewport(extent);
    _createInfo._scissor         = vkl::init::rect2D(extent);

    _createInfo._dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    _createInfo._dynamicState  = vkl::init::pipelineDynamicStateCreateInfo(_createInfo._dynamicStages.data(), static_cast<uint32_t>(_createInfo._dynamicStages.size()));

    _createInfo._rasterizer           = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    _createInfo._multisampling        = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    _createInfo._colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    _createInfo._depthStencil         = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
}
} // namespace vkl
