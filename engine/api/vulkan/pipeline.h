#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "api/gpuResource.h"
#include "shader.h"
#include "vkUtils.h"

namespace aph::vk
{
class Device;
class DescriptorSetLayout;

enum class VertexComponent
{
    POSITION,
    NORMAL,
    UV,
    COLOR,
    TANGENT,
};

struct GraphicsPipelineCreateInfo
{
    std::vector<VkVertexInputBindingDescription>     inputBinding          = {};
    std::vector<VkVertexInputAttributeDescription>   inputAttribute        = {};
    VkPipelineVertexInputStateCreateInfo             vertexInputInfo       = {};
    VkPipelineRenderingCreateInfo                    renderingCreateInfo   = {};
    std::vector<VkDynamicState>                      dynamicStages         = {};
    VkPipelineInputAssemblyStateCreateInfo           inputAssembly         = {};
    VkViewport                                       viewport              = {};
    VkRect2D                                         scissor               = {};
    VkPipelineDynamicStateCreateInfo                 dynamicState          = {};
    VkPipelineRasterizationStateCreateInfo           rasterizer            = {};
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {};
    VkPipelineMultisampleStateCreateInfo             multisampling         = {};
    VkPipelineDepthStencilStateCreateInfo            depthStencil          = {};
    VkPipelineCache                                  pipelineCache         = {};

    GraphicsPipelineCreateInfo(const std::vector<VertexComponent>& component = {VertexComponent::POSITION,
                                                                                VertexComponent::NORMAL,
                                                                                VertexComponent::UV,
                                                                                VertexComponent::COLOR,
                                                                                VertexComponent::TANGENT},
                               VkExtent2D                          extent    = {0, 0});
};

struct ComputePipelineCreateInfo
{
};

class Pipeline : public ResourceHandle<VkPipeline>
{
public:
    Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, ShaderProgram* program, VkPipeline handle);
    Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, ShaderProgram* program, VkPipeline handle);

    ShaderProgram*      getProgram() { return m_pProgram; }
    VkPipelineLayout    getPipelineLayout() { return m_pProgram->m_pipeLayout; }
    VkPipelineBindPoint getBindPoint() { return m_bindPoint; }

    VkShaderStageFlags getConstantShaderStage(uint32_t offset, uint32_t size);

protected:
    Device*             m_pDevice   = {};
    ShaderProgram*      m_pProgram  = {};
    VkPipelineBindPoint m_bindPoint = {};
    VkPipelineCache     m_cache     = {};
};

}  // namespace aph::vk

#endif  // PIPELINE_H_
