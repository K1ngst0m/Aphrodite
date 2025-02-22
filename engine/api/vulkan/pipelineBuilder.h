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

private:
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

    enum
    {
        APH_MAX_DYNAMIC_STATES = 128
    };
    uint32_t       numDynamicStates_                      = 0;
    VkDynamicState dynamicStates_[APH_MAX_DYNAMIC_STATES] = {};

    SmallVector<VkPipelineShaderStageCreateInfo> shaderStages_ = {};

    VkPipelineVertexInputStateCreateInfo   vertexInputState_;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_;
    VkPipelineRasterizationStateCreateInfo rasterizationState_;
    VkPipelineMultisampleStateCreateInfo   multisampleState_;
    VkPipelineDepthStencilStateCreateInfo  depthStencilState_;

    VkPipelineDynamicStateCreateInfo    dynamicState_;
    VkPipelineViewportStateCreateInfo   viewportState_;
    VkPipelineColorBlendStateCreateInfo colorBlendState_;
    VkPipelineRenderingCreateInfo       renderingInfo_;
    VkPipelineCreateFlags2CreateInfoKHR createFlags_;

    SmallVector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates_ = {};
    SmallVector<VkFormat>                            colorAttachmentFormats_     = {};

    SmallVector<VkVertexInputBindingDescription>   vkBindings_;
    SmallVector<VkVertexInputAttributeDescription> vkAttributes_;

    VkFormat depthAttachmentFormat_   = VK_FORMAT_UNDEFINED;
    VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

    static uint32_t numPipelinesCreated_;
};

}  // namespace aph::vk
