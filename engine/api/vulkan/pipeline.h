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
    ShaderProgram*                                   pProgram              = {};

    GraphicsPipelineCreateInfo(const std::vector<VertexComponent>& component = {VertexComponent::POSITION,
                                                                                VertexComponent::NORMAL,
                                                                                VertexComponent::UV,
                                                                                VertexComponent::COLOR,
                                                                                VertexComponent::TANGENT},
                               VkExtent2D                          extent    = {0, 0});
};

struct ComputePipelineCreateInfo
{
    ShaderProgram* pProgram = {};
};

class Pipeline : public ResourceHandle<VkPipeline>
{
public:
    Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, VkPipeline handle);
    Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, VkPipeline handle);

    ShaderProgram*      getProgram() { return m_pProgram; }
    VkPipelineBindPoint getBindPoint() { return m_bindPoint; }

protected:
    Device*             m_pDevice   = {};
    ShaderProgram*      m_pProgram  = {};
    VkPipelineBindPoint m_bindPoint = {};
    VkPipelineCache     m_cache     = {};
};

}  // namespace aph::vk

#endif  // PIPELINE_H_
