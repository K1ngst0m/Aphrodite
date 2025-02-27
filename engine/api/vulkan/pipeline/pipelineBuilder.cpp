#include "pipelineBuilder.h"
#include "vkUtils.h"
#include "device.h"

namespace aph::vk
{
VkGraphicsPipelineCreateInfo VulkanPipelineBuilder::getCreateInfo(const GraphicsPipelineCreateInfo& createInfo)
{
    auto& pProgram = createInfo.pProgram;
    APH_ASSERT(pProgram);

    // Not all attachments are valid. We need to create color blend attachments only for active attachments
    const uint32_t numColorAttachments = createInfo.color.size();

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates{numColorAttachments};
    std::vector<VkFormat>                            colorAttachmentFormats{numColorAttachments};
    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        const auto& attachment = createInfo.color[i];
        APH_ASSERT(attachment.format != Format::Undefined);
        colorAttachmentFormats[i] = utils::VkCast(attachment.format);
        if(!attachment.blendEnabled)
        {
            colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable         = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
        }
        else
        {
            colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable         = VK_TRUE,
                .srcColorBlendFactor = utils::VkCast(attachment.srcRGBBlendFactor),
                .dstColorBlendFactor = utils::VkCast(attachment.dstRGBBlendFactor),
                .colorBlendOp        = utils::VkCast(attachment.rgbBlendOp),
                .srcAlphaBlendFactor = utils::VkCast(attachment.srcAlphaBlendFactor),
                .dstAlphaBlendFactor = utils::VkCast(attachment.dstAlphaBlendFactor),
                .alphaBlendOp        = utils::VkCast(attachment.alphaBlendOp),
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
        }
    }

    // from Vulkan 1.0
    dynamicState(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT)
        .dynamicState(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT)
        .dynamicState(VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS)
        .dynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS)
        // from Vulkan 1.3
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP)
        .depthBiasEnable(createInfo.dynamicState.depthBiasEnable)
        .rasterizationSamples(aph::vk::utils::getSampleCountFlags(createInfo.samplesCount))
        .polygonMode(utils::VkCast(createInfo.polygonMode))
        .stencilStateOps(VK_STENCIL_FACE_FRONT_BIT, utils::VkCast(createInfo.frontFaceStencil.stencilFailureOp),
                         utils::VkCast(createInfo.frontFaceStencil.depthStencilPassOp),
                         utils::VkCast(createInfo.frontFaceStencil.depthFailureOp),
                         utils::VkCast(createInfo.frontFaceStencil.stencilCompareOp))
        .stencilStateOps(VK_STENCIL_FACE_BACK_BIT, utils::VkCast(createInfo.backFaceStencil.stencilFailureOp),
                         utils::VkCast(createInfo.backFaceStencil.depthStencilPassOp),
                         utils::VkCast(createInfo.backFaceStencil.depthFailureOp),
                         utils::VkCast(createInfo.backFaceStencil.stencilCompareOp))
        .stencilMasks(VK_STENCIL_FACE_FRONT_BIT, 0xFF, createInfo.frontFaceStencil.writeMask,
                      createInfo.frontFaceStencil.readMask)
        .stencilMasks(VK_STENCIL_FACE_BACK_BIT, 0xFF, createInfo.backFaceStencil.writeMask,
                      createInfo.backFaceStencil.readMask)
        .cullMode(utils::VkCast(createInfo.cullMode))
        .frontFace(utils::VkCast(createInfo.frontFaceWinding))
        .colorAttachments(colorBlendAttachmentStates.data(), colorAttachmentFormats.data(), numColorAttachments)
        .depthAttachmentFormat(utils::VkCast(createInfo.depthFormat))
        .stencilAttachmentFormat(utils::VkCast(createInfo.stencilFormat));

    SmallVector<VkVertexInputBindingDescription>&   vkBindings   = m_vkBindings;
    SmallVector<VkVertexInputAttributeDescription>& vkAttributes = m_vkAttributes;
    switch(createInfo.type)
    {
    case PipelineType::Geometry:
    {
        const Shader* vs = pProgram->getShader(ShaderStage::VS);
        shaderStage(
            init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs->getHandle(), vs->getEntryPointName()));
        const VertexInput& vstate = createInfo.vertexInput;
        vkAttributes.resize(vstate.attributes.size());
        SmallVector<bool> bufferAlreadyBound(vstate.bindings.size());

        for(uint32_t i = 0; i != vkAttributes.size(); i++)
        {
            const auto& attr = vstate.attributes[i];

            vkAttributes[i] = {.location = attr.location,
                               .binding  = attr.binding,
                               .format   = utils::VkCast(attr.format),
                               .offset   = (uint32_t)attr.offset};

            if(!bufferAlreadyBound[attr.binding])
            {
                bufferAlreadyBound[attr.binding] = true;
                vkBindings.push_back({.binding   = attr.binding,
                                      .stride    = vstate.bindings[attr.binding].stride,
                                      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});
            }
        }
        const VkPipelineVertexInputStateCreateInfo ciVertexInputState = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<uint32_t>(vkBindings.size()),
            .pVertexBindingDescriptions      = !vkBindings.empty() ? vkBindings.data() : nullptr,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size()),
            .pVertexAttributeDescriptions    = !vkAttributes.empty() ? vkAttributes.data() : nullptr,
        };
        primitiveTopology(utils::VkCast(createInfo.topology));
        vertexInputState(ciVertexInputState);
    }
    break;
    case PipelineType::Mesh:
    {
        const Shader* ms = pProgram->getShader(ShaderStage::MS);
        shaderStage(init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_MESH_BIT_EXT, ms->getHandle(),
                                                        ms->getEntryPointName()));
        if(auto ts = pProgram->getShader(ShaderStage::TS); ts != nullptr)
        {
            shaderStage(init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_TASK_BIT_EXT, ts->getHandle(),
                                                            ts->getEntryPointName()));
        }
    }
    break;
    case PipelineType::Compute:
    case PipelineType::RayTracing:
    default:
        APH_ASSERT(false);
        return {};
    }

    const Shader* fs = pProgram->getShader(ShaderStage::FS);
    shaderStage(
        init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fs->getHandle(), fs->getEntryPointName()));

    m_dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size()),
        .pDynamicStates    = m_dynamicStates.data(),
    };
    // viewport and scissor can be NULL if the viewport state is dynamic
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineViewportStateCreateInfo.html
    m_viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 0,
        .pViewports    = nullptr,
        .scissorCount  = 0,
        .pScissors     = nullptr,
    };

    m_colorBlendState = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(m_colorBlendAttachmentStates.size()),
        .pAttachments    = m_colorBlendAttachmentStates.data(),
    };

    m_renderingInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext                   = nullptr,
        .colorAttachmentCount    = static_cast<uint32_t>(m_colorAttachmentFormats.size()),
        .pColorAttachmentFormats = m_colorAttachmentFormats.data(),
        .depthAttachmentFormat   = m_depthAttachmentFormat,
        .stencilAttachmentFormat = m_stencilAttachmentFormat,
    };

    m_createFlags = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO_KHR,
        .pNext = &m_renderingInfo,
        .flags = VK_PIPELINE_CREATE_2_CAPTURE_DATA_BIT_KHR,
    };
    bool                         isGeometryPipeline = createInfo.type == PipelineType::Geometry;
    VkGraphicsPipelineCreateInfo graphicsCreateInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &m_createFlags,
        .flags               = 0,
        .stageCount          = static_cast<uint32_t>(m_shaderStages.size()),
        .pStages             = m_shaderStages.data(),
        .pVertexInputState   = isGeometryPipeline ? &m_vertexInputState : nullptr,
        .pInputAssemblyState = isGeometryPipeline ? &m_inputAssembly : nullptr,
        .pTessellationState  = nullptr,
        .pViewportState      = &m_viewportState,
        .pRasterizationState = &m_rasterizationState,
        .pMultisampleState   = &m_multisampleState,
        .pDepthStencilState  = &m_depthStencilState,
        .pColorBlendState    = &m_colorBlendState,
        .pDynamicState       = &m_dynamicState,
        .layout              = pProgram->getPipelineLayout(),
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };
    return graphicsCreateInfo;
}

VulkanPipelineBuilder::VulkanPipelineBuilder() :
    m_vertexInputState(VkPipelineVertexInputStateCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr,
    }),
    m_inputAssembly({
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    }),
    m_rasterizationState({
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
    m_multisampleState({
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    }),
    m_depthStencilState({
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
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
    m_rasterizationState.depthBiasEnable = enable ? VK_TRUE : VK_FALSE;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::dynamicState(VkDynamicState state)
{
    m_dynamicStates.push_back(state);
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::primitiveTopology(VkPrimitiveTopology topology)
{
    m_inputAssembly.topology = topology;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::rasterizationSamples(VkSampleCountFlagBits samples)
{
    m_multisampleState.rasterizationSamples = samples;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::cullMode(VkCullModeFlags mode)
{
    m_rasterizationState.cullMode = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::frontFace(VkFrontFace mode)
{
    m_rasterizationState.frontFace = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::polygonMode(VkPolygonMode mode)
{
    m_rasterizationState.polygonMode = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::vertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
    m_vertexInputState = state;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::colorAttachments(const VkPipelineColorBlendAttachmentState* states,
                                                               const VkFormat* formats, uint32_t numColorAttachments)
{
    APH_ASSERT(states);
    APH_ASSERT(formats);
    m_colorBlendAttachmentStates.resize(numColorAttachments);
    m_colorAttachmentFormats.resize(numColorAttachments);
    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        m_colorBlendAttachmentStates[i] = states[i];
        m_colorAttachmentFormats[i]     = formats[i];
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthAttachmentFormat(VkFormat format)
{
    m_depthAttachmentFormat = format;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilAttachmentFormat(VkFormat format)
{
    m_stencilAttachmentFormat = format;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage)
{
    if(stage.module != VK_NULL_HANDLE)
    {
        m_shaderStages.push_back(stage);
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(const SmallVector<VkPipelineShaderStageCreateInfo>& stages)
{
    for(auto stage : stages)
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
        VkStencilOpState& s = m_depthStencilState.front;
        s.failOp            = failOp;
        s.passOp            = passOp;
        s.depthFailOp       = depthFailOp;
        s.compareOp         = compareOp;
    }

    if(faceMask & VK_STENCIL_FACE_BACK_BIT)
    {
        VkStencilOpState& s = m_depthStencilState.back;
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
        VkStencilOpState& s = m_depthStencilState.front;
        s.compareMask       = compareMask;
        s.writeMask         = writeMask;
        s.reference         = reference;
    }

    if(faceMask & VK_STENCIL_FACE_BACK_BIT)
    {
        VkStencilOpState& s = m_depthStencilState.back;
        s.compareMask       = compareMask;
        s.writeMask         = writeMask;
        s.reference         = reference;
    }
    return *this;
}

VkResult VulkanPipelineBuilder::build(vk::Device* pDevice, const GraphicsPipelineCreateInfo& createInfo,
                                      VkPipeline* outPipeline) noexcept
{
    const auto& ci     = getCreateInfo(createInfo);
    const auto  result = pDevice->getDeviceTable()->vkCreateGraphicsPipelines(pDevice->getHandle(), VK_NULL_HANDLE, 1,
                                                                              &ci, vkAllocator(), outPipeline);

    if(result != VK_SUCCESS)
    {
        return result;
    }

    return VK_SUCCESS;
}
}  // namespace aph::vk
