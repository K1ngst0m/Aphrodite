#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "device.h"

namespace vkl {

enum class VertexComponent {
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

struct VertexInputBuilder {
    std::vector<VkVertexInputBindingDescription>   _vertexInputBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
    VkPipelineVertexInputStateCreateInfo           _pipelineVertexInputStateCreateInfo;
    VkPipelineVertexInputStateCreateInfo&          getPipelineVertexInputState(const std::vector<VertexComponent> &components);
};

struct GraphicsPipelineCreateInfo {
    std::vector<VkDynamicState>            dynamicStages;
    VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkViewport                             viewport;
    VkRect2D                               scissor;
    VkPipelineDynamicStateCreateInfo       dynamicState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState    colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo   multisampling;
    VertexInputBuilder                     vertexInputBuilder;
    VkPipelineDepthStencilStateCreateInfo  depthStencil;
    VkPipelineCache                        pipelineCache = VK_NULL_HANDLE;

    GraphicsPipelineCreateInfo(VkExtent2D extent = {0, 0}) {
        vertexInputInfo = vertexInputBuilder.getPipelineVertexInputState({VertexComponent::POSITION,
                                                                            VertexComponent::NORMAL,
                                                                            VertexComponent::UV,
                                                                            VertexComponent::COLOR,
                                                                            VertexComponent::TANGENT});

        inputAssembly = vkl::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        dynamicState  = vkl::init::pipelineDynamicStateCreateInfo(dynamicStages.data(), static_cast<uint32_t>(dynamicStages.size()));

        rasterizer           = vkl::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        multisampling        = vkl::init::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        colorBlendAttachment = vkl::init::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        depthStencil         = vkl::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    }
};

class VulkanPipeline : public ResourceHandle<VkPipeline> {
public:
    static VulkanPipeline *CreateGraphicsPipeline(VulkanDevice             *pDevice,
                                                  const GraphicsPipelineCreateInfo *pCreateInfo,
                                                  ShaderEffect             *effect,
                                                  VulkanRenderPass         *pRenderPass,
                                                  VkPipeline                handle);

    static VulkanPipeline *CreateComputePipeline(VulkanDevice *pDevice,
                                                 ShaderEffect * pEffect,
                                                 VkPipeline handle);

    VkPipelineLayout           getPipelineLayout();
    VulkanDescriptorSetLayout *getDescriptorSetLayout(uint32_t idx);
    ShaderEffect              *getEffect();
    VkPipelineBindPoint        getBindPoint();

protected:
    VkPipelineCache    _cache;
    VulkanDevice      *_device = nullptr;
    ShaderEffect      *_effect = nullptr;
    VkPipelineBindPoint _bindPoint;
};

} // namespace vkl

#endif // PIPELINE_H_
