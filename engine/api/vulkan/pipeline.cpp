#include "pipeline.h"
#include "scene/mesh.h"
#include "device.h"

namespace aph::vk
{
namespace
{
std::unordered_map<VertexComponent, VkVertexInputAttributeDescription> vertexComponmentMap{
    {VertexComponent::POSITION, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)}},
    {VertexComponent::NORMAL, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}},
    {VertexComponent::UV, {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}},
    {VertexComponent::COLOR, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}},
    {VertexComponent::TANGENT, {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}},
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
        for(const VertexComponent& component : components)
        {
            VkVertexInputAttributeDescription desc = vertexComponmentMap[component];
            desc.location                          = location;
            inputAttribute.push_back(desc);
            location++;
            bindingSize += vertexComponentSizeMap[component];
        }
        inputBinding    = {{0, bindingSize, VK_VERTEX_INPUT_RATE_VERTEX}};
        vertexInputInfo = aph::vk::init::pipelineVertexInputStateCreateInfo(inputBinding, inputAttribute);
    }
    inputAssembly = init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    dynamicStages = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    dynamicState =
        init::pipelineDynamicStateCreateInfo(dynamicStages.data(), static_cast<uint32_t>(dynamicStages.size()));
    rasterizer    = init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                               VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    multisampling = init::pipelineMultisampleStateCreateInfo();
    colorBlendAttachments.push_back(init::pipelineColorBlendAttachmentState());
    depthStencil = init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
}

Pipeline::Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, ShaderProgram* program,
                   VkPipeline handle) :
    m_pDevice(pDevice),
    m_pProgram(program),
    m_bindPoint(VK_PIPELINE_BIND_POINT_COMPUTE)
{
    getHandle() = handle;
}

Pipeline::Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, ShaderProgram* program,
                   VkPipeline handle) :
    m_pDevice(pDevice),
    m_pProgram(program),
    m_bindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS)
{
    getHandle() = handle;
}

VkShaderStageFlags Pipeline::getConstantShaderStage(uint32_t offset, uint32_t size)
{
    VkShaderStageFlags stage = 0;
    size += offset;
    const auto& constant = m_pProgram->m_combineLayout.pushConstantRange;
    stage |= constant.stageFlags;
    offset += constant.size;
    return stage;
}

}  // namespace aph::vk
