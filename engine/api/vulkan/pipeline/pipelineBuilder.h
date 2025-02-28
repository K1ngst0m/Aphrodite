#pragma once

#include "pipeline.h"

namespace aph::vk
{
class VulkanPipelineBuilder final
{
public:
    VulkanPipelineBuilder();
    ~VulkanPipelineBuilder() = default;

    VkResult build(vk::Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, VkPipeline* outPipeline) noexcept;
    VkGraphicsPipelineCreateInfo getCreateInfo(const GraphicsPipelineCreateInfo& createInfo);

    VulkanPipelineBuilder& depthBiasEnable(bool enable);
    VulkanPipelineBuilder& dynamicState(VkDynamicState state);
    VulkanPipelineBuilder& primitiveTopology(VkPrimitiveTopology topology);
    VulkanPipelineBuilder& rasterizationSamples(VkSampleCountFlagBits samples);
    VulkanPipelineBuilder& shaderStage(VkPipelineShaderStageCreateInfo stage);
    VulkanPipelineBuilder& shaderStage(const SmallVector<VkPipelineShaderStageCreateInfo>& stages);
    VulkanPipelineBuilder& stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp,
                                           VkStencilOp depthFailOp, VkCompareOp compareOp);
    VulkanPipelineBuilder& stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask, uint32_t writeMask,
                                        uint32_t reference);
    VulkanPipelineBuilder& cullMode(VkCullModeFlags mode);
    VulkanPipelineBuilder& frontFace(VkFrontFace mode);
    VulkanPipelineBuilder& polygonMode(VkPolygonMode mode);
    VulkanPipelineBuilder& vertexInputState(const VkPipelineVertexInputStateCreateInfo& state);
    VulkanPipelineBuilder& colorAttachments(const VkPipelineColorBlendAttachmentState* states, const VkFormat* formats,
                                            uint32_t numColorAttachments);
    VulkanPipelineBuilder& depthAttachmentFormat(VkFormat format);
    VulkanPipelineBuilder& stencilAttachmentFormat(VkFormat format);

private:
    std::vector<VkDynamicState> m_dynamicStates = {};

    SmallVector<VkPipelineShaderStageCreateInfo> m_shaderStages = {};

    VkPipelineVertexInputStateCreateInfo m_vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizationState;
    VkPipelineMultisampleStateCreateInfo m_multisampleState;
    VkPipelineDepthStencilStateCreateInfo m_depthStencilState;

    VkPipelineDynamicStateCreateInfo m_dynamicState;
    VkPipelineViewportStateCreateInfo m_viewportState;
    VkPipelineColorBlendStateCreateInfo m_colorBlendState;
    VkPipelineRenderingCreateInfo m_renderingInfo;
    VkPipelineCreateFlags2CreateInfoKHR m_createFlags;

    SmallVector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates = {};
    SmallVector<VkFormat> m_colorAttachmentFormats = {};

    SmallVector<VkVertexInputBindingDescription> m_vkBindings;
    SmallVector<VkVertexInputAttributeDescription> m_vkAttributes;

    VkFormat m_depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkFormat m_stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
};

} // namespace aph::vk
