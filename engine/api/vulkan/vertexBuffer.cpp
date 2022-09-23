#include "vertexBuffer.h"
#include "scene/entity.h"

namespace vkl {
VkVertexInputBindingDescription VertexInputBuilder::_vertexInputBindingDescription;
std::vector<VkVertexInputAttributeDescription> VertexInputBuilder::_vertexInputAttributeDescriptions;
VkPipelineVertexInputStateCreateInfo VertexInputBuilder::_pipelineVertexInputStateCreateInfo;
VkVertexInputAttributeDescription VertexInputBuilder::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component)
{
    switch (component) {
    case VertexComponent::POSITION:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, pos) });
    case VertexComponent::NORMAL:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, normal) });
    case VertexComponent::UV:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexLayout, uv) });
    case VertexComponent::COLOR:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, color) });
    case VertexComponent::TANGENT:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexLayout, tangent) });
    default:
        return VkVertexInputAttributeDescription({});
    }
}
std::vector<VkVertexInputAttributeDescription> VertexInputBuilder::inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components)
{
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t location = 0;
    for (VertexComponent component : components) {
        result.push_back(VertexInputBuilder::inputAttributeDescription(binding, location, component));
        location++;
    }
    return result;
}
void VertexInputBuilder::setPipelineVertexInputState(const std::vector<VertexComponent> &components)
{
    _vertexInputBindingDescription = VkVertexInputBindingDescription({ 0, sizeof(VertexLayout), VK_VERTEX_INPUT_RATE_VERTEX });
    _vertexInputAttributeDescriptions = VertexInputBuilder::inputAttributeDescriptions(0, components);
    _pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &VertexInputBuilder::_vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexInputBuilder::_vertexInputAttributeDescriptions.size()),
        .pVertexAttributeDescriptions = VertexInputBuilder::_vertexInputAttributeDescriptions.data(),
    };
}
}
