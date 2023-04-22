#include "pipeline.h"
#include "scene/mesh.h"

namespace aph
{
namespace
{
std::unordered_map<VertexComponent, VkVertexInputAttributeDescription> vertexComponmentMap{
    {VertexComponent::POSITION, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)}},
    {VertexComponent::NORMAL, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}},
    {VertexComponent::UV, {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}},
    {VertexComponent::COLOR, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}},
    {VertexComponent::TANGENT, {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)}},
};

std::unordered_map<VertexComponent, size_t> vertexComponentSizeMap{
    {VertexComponent::POSITION, {sizeof(Vertex::pos)}},    {VertexComponent::NORMAL, {sizeof(Vertex::normal)}},
    {VertexComponent::UV, {sizeof(Vertex::uv)}},           {VertexComponent::COLOR, {sizeof(Vertex::color)}},
    {VertexComponent::TANGENT, {sizeof(Vertex::tangent)}},
};
}  // namespace

GraphicsPipelineCreateInfo::GraphicsPipelineCreateInfo(const std::vector<VertexComponent>& components,
                                                       VkExtent2D                          extent)
{
    {
        uint32_t location    = 0;
        uint32_t bindingSize = 0;
        for(VertexComponent component : components)
        {
            VkVertexInputAttributeDescription desc = vertexComponmentMap[component];
            desc.location                          = location;
            inputAttribute.push_back(desc);
            location++;
            bindingSize += vertexComponentSizeMap[component];
        }
        inputBinding    = {{0, bindingSize, VK_VERTEX_INPUT_RATE_VERTEX}};
        vertexInputInfo = aph::init::pipelineVertexInputStateCreateInfo(inputBinding, inputAttribute);
    }
    inputAssembly = aph::init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    dynamicState =
        aph::init::pipelineDynamicStateCreateInfo(dynamicStages.data(), static_cast<uint32_t>(dynamicStages.size()));
    rasterizer           = aph::init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                                           VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    multisampling        = aph::init::pipelineMultisampleStateCreateInfo();
    colorBlendAttachment = aph::init::pipelineColorBlendAttachmentState();
    depthStencil         = aph::init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
}

VulkanPipeline::VulkanPipeline(VulkanDevice* pDevice, const ComputePipelineCreateInfo& createInfo,
                               VkPipelineLayout layout, VkPipeline handle) :
    m_pDevice(pDevice),
    m_pipelineLayout(layout),
    m_bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE),
    m_constants(createInfo.constants),
    m_setLayouts(createInfo.setLayouts),
    m_shaderMapList(createInfo.shaderMapList)
{
    getHandle() = handle;
}

VulkanPipeline::VulkanPipeline(VulkanDevice* pDevice, const GraphicsPipelineCreateInfo& createInfo,
                               VkRenderPass renderPass, VkPipelineLayout layout, VkPipeline handle) :
    m_pDevice(pDevice),
    m_renderPass(renderPass),
    m_pipelineLayout(layout),
    m_bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS),
    m_constants(createInfo.constants),
    m_setLayouts(createInfo.setLayouts),
    m_shaderMapList(createInfo.shaderMapList)
{
    getHandle() = handle;
}

}  // namespace aph
