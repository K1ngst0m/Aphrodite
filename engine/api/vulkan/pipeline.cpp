#include "pipeline.h"
#include "scene/mesh.h"
#include "device.h"
#include "shader.h"

namespace aph::vk
{
uint32_t VulkanPipelineBuilder::numPipelinesCreated_ = 0;

VulkanPipelineBuilder::VulkanPipelineBuilder() :
    vertexInputState_(VkPipelineVertexInputStateCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr,
    }),
    inputAssembly_({
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    }),
    rasterizationState_({
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    }),
    multisampleState_({
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    }),
    depthStencilState_({
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .flags                 = 0,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front =
            {
                .failOp      = VK_STENCIL_OP_KEEP,
                .passOp      = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp   = VK_COMPARE_OP_NEVER,
                .compareMask = 0,
                .writeMask   = 0,
                .reference   = 0,
            },
        .back =
            {
                .failOp      = VK_STENCIL_OP_KEEP,
                .passOp      = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp   = VK_COMPARE_OP_NEVER,
                .compareMask = 0,
                .writeMask   = 0,
                .reference   = 0,
            },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    })
{
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthBiasEnable(bool enable)
{
    rasterizationState_.depthBiasEnable = enable ? VK_TRUE : VK_FALSE;
    return *this;
}

#if defined(__APPLE__)
VulkanPipelineBuilder& VulkanPipelineBuilder::depthWriteEnable(bool enable)
{
    depthStencilState_.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthCompareOp(VkCompareOp compareOp)
{
    depthStencilState_.depthTestEnable = compareOp != VK_COMPARE_OP_ALWAYS;
    depthStencilState_.depthCompareOp  = compareOp;
    return *this;
}
#endif

VulkanPipelineBuilder& VulkanPipelineBuilder::dynamicState(VkDynamicState state)
{
    APH_ASSERT(numDynamicStates_ < APH_MAX_DYNAMIC_STATES);
    dynamicStates_[numDynamicStates_++] = state;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::primitiveTopology(VkPrimitiveTopology topology)
{
    inputAssembly_.topology = topology;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::rasterizationSamples(VkSampleCountFlagBits samples)
{
    multisampleState_.rasterizationSamples = samples;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::cullMode(VkCullModeFlags mode)
{
    rasterizationState_.cullMode = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::frontFace(VkFrontFace mode)
{
    rasterizationState_.frontFace = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::polygonMode(VkPolygonMode mode)
{
    rasterizationState_.polygonMode = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::vertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
    vertexInputState_ = state;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::colorAttachments(const VkPipelineColorBlendAttachmentState* states,
                                                               const VkFormat* formats, uint32_t numColorAttachments)
{
    APH_ASSERT(states);
    APH_ASSERT(formats);
    colorBlendAttachmentStates_.resize(numColorAttachments);
    colorAttachmentFormats_.resize(numColorAttachments);
    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        colorBlendAttachmentStates_[i] = states[i];
        colorAttachmentFormats_[i]     = formats[i];
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthAttachmentFormat(VkFormat format)
{
    depthAttachmentFormat_ = format;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilAttachmentFormat(VkFormat format)
{
    stencilAttachmentFormat_ = format;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage)
{
    if(stage.module != VK_NULL_HANDLE)
    {
        shaderStages_.push_back(stage);
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(const std::vector<VkPipelineShaderStageCreateInfo>& stages)
{
    for (auto stage : stages)
    {
        shaderStage(stage);
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp,
                                                              VkStencilOp passOp, VkStencilOp depthFailOp,
                                                              VkCompareOp compareOp)
{
    if(faceMask & VK_STENCIL_FACE_FRONT_BIT)
    {
        VkStencilOpState& s = depthStencilState_.front;
        s.failOp            = failOp;
        s.passOp            = passOp;
        s.depthFailOp       = depthFailOp;
        s.compareOp         = compareOp;
    }

    if(faceMask & VK_STENCIL_FACE_BACK_BIT)
    {
        VkStencilOpState& s = depthStencilState_.back;
        s.failOp            = failOp;
        s.passOp            = passOp;
        s.depthFailOp       = depthFailOp;
        s.compareOp         = compareOp;
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask,
                                                           uint32_t writeMask, uint32_t reference)
{
    if(faceMask & VK_STENCIL_FACE_FRONT_BIT)
    {
        VkStencilOpState& s = depthStencilState_.front;
        s.compareMask       = compareMask;
        s.writeMask         = writeMask;
        s.reference         = reference;
    }

    if(faceMask & VK_STENCIL_FACE_BACK_BIT)
    {
        VkStencilOpState& s = depthStencilState_.back;
        s.compareMask       = compareMask;
        s.writeMask         = writeMask;
        s.reference         = reference;
    }
    return *this;
}

VkResult VulkanPipelineBuilder::build(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout,
                                      VkPipeline* outPipeline, const char* debugName) noexcept
{
    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = numDynamicStates_,
        .pDynamicStates    = dynamicStates_,
    };
    // viewport and scissor can be NULL if the viewport state is dynamic
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineViewportStateCreateInfo.html
    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = nullptr,
        .scissorCount  = 1,
        .pScissors     = nullptr,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(colorBlendAttachmentStates_.size()),
        .pAttachments    = colorBlendAttachmentStates_.data(),
    };
    const VkPipelineRenderingCreateInfo renderingInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext                   = nullptr,
        .colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentFormats_.size()),
        .pColorAttachmentFormats = colorAttachmentFormats_.data(),
        .depthAttachmentFormat   = depthAttachmentFormat_,
        .stencilAttachmentFormat = stencilAttachmentFormat_,
    };

    const VkGraphicsPipelineCreateInfo ci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &renderingInfo,
        .flags               = 0,
        .stageCount          = static_cast<uint32_t>(shaderStages_.size()),
        .pStages             = shaderStages_.data(),
        .pVertexInputState   = &vertexInputState_,
        .pInputAssemblyState = &inputAssembly_,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizationState_,
        .pMultisampleState   = &multisampleState_,
        .pDepthStencilState  = &depthStencilState_,
        .pColorBlendState    = &colorBlendState,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    const auto result = vkCreateGraphicsPipelines(device, pipelineCache, 1, &ci, nullptr, outPipeline);

    if(result != VK_SUCCESS)
    {
        return result;
    }

    numPipelinesCreated_++;

    return VK_SUCCESS;
}

Pipeline::Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, VkPipeline handle, ShaderProgram* pProgram) :
    m_pDevice(pDevice),
    m_pProgram(pProgram),
    m_bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE)
{
    APH_ASSERT(pProgram);
    getHandle() = handle;
}

Pipeline::Pipeline(Device* pDevice, const RenderPipelineState& rps, VkPipeline handle, ShaderProgram* pProgram) :
    m_pDevice(pDevice),
    m_pProgram(pProgram),
    m_bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS),
    m_rps(rps)
{
    APH_ASSERT(pProgram);
    getHandle() = handle;
}

DescriptorSet* Pipeline::acquireSet(uint32_t idx) const
{
    return m_pProgram->getSetLayout(idx)->allocateSet();
}

}  // namespace aph::vk
