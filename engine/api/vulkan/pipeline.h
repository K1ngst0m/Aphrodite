#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "api/gpuResource.h"
#include "vkUtils.h"
#include "shader.h"

namespace aph
{
class VulkanDevice;
class VulkanDescriptorSetLayout;

enum class VertexComponent
{
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

enum class VertexFormat
{
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
};

struct VertexInputBuilder
{
    std::vector<VkVertexInputBindingDescription> inputBinding;
    std::vector<VkVertexInputAttributeDescription> inputAttribute;
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineVertexInputStateCreateInfo &getPipelineVertexInputState(const std::vector<VertexComponent> &components);
};

struct GraphicsPipelineCreateInfo
{
    VkPipelineRenderingCreateInfo renderingCreateInfo;
    std::vector<VkDynamicState> dynamicStages;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VertexInputBuilder vertexInputBuilder;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;

    std::vector<VulkanDescriptorSetLayout *> setLayouts;
    std::vector<VkPushConstantRange> constants;
    ShaderMapList shaderMapList;

    GraphicsPipelineCreateInfo(const std::vector<VertexComponent> &component, VkExtent2D extent = { 0, 0 })
    {
        vertexInputInfo = vertexInputBuilder.getPipelineVertexInputState(component);
        inputAssembly = aph::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        dynamicStages = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        dynamicState = aph::init::pipelineDynamicStateCreateInfo(dynamicStages.data(),
                                                                 static_cast<uint32_t>(dynamicStages.size()));
        rasterizer = aph::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                                     VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        multisampling = aph::init::pipelineMultisampleStateCreateInfo();
        colorBlendAttachment = aph::init::pipelineColorBlendAttachmentState();
        depthStencil = aph::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    }

    GraphicsPipelineCreateInfo() :
        GraphicsPipelineCreateInfo({ VertexComponent::POSITION, VertexComponent::NORMAL, VertexComponent::UV,
                                     VertexComponent::COLOR, VertexComponent::TANGENT })
    {}
};

struct ComputePipelineCreateInfo
{
    std::vector<VulkanDescriptorSetLayout *> setLayouts;
    std::vector<VkPushConstantRange> constants;
    ShaderMapList shaderMapList;
};

class VulkanPipeline : public ResourceHandle<VkPipeline>
{
public:
    static VulkanPipeline *CreateGraphicsPipeline(VulkanDevice *pDevice, const GraphicsPipelineCreateInfo &createInfo,
                                                  VkRenderPass renderPass, VkPipelineLayout layout,
                                                  VkPipeline handle);

    static VulkanPipeline *CreateComputePipeline(VulkanDevice *pDevice, const ComputePipelineCreateInfo &createInfo, VkPipelineLayout layout,
                                                 VkPipeline handle);

    VulkanDescriptorSetLayout *getDescriptorSetLayout(uint32_t idx) { return m_setLayouts[idx]; }
    VkPipelineLayout getPipelineLayout() { return m_layout; }
    VkPipelineBindPoint getBindPoint() { return m_bindPoint; }

protected:
    VkRenderPass m_renderPass{};
    VkPipelineCache m_cache {};
    VulkanDevice *m_device {};
    VkPipelineLayout m_layout {};
    VkPipelineBindPoint m_bindPoint {};
    std::vector<VkPushConstantRange> m_constants;
    std::vector<VulkanDescriptorSetLayout *> m_setLayouts;
    ShaderMapList m_shaderMapList;
};

}  // namespace aph

#endif  // PIPELINE_H_
