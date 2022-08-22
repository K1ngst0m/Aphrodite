#include "vklMesh.h"

namespace vkl {

VkVertexInputBindingDescription VertexLayout::_vertexInputBindingDescription;
std::vector<VkVertexInputAttributeDescription> VertexLayout::_vertexInputAttributeDescriptions;
VkPipelineVertexInputStateCreateInfo VertexLayout::_pipelineVertexInputStateCreateInfo;

VkVertexInputAttributeDescription VertexLayout::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component)
{
    switch (component) {
    case VertexComponent::POSITION:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, pos) });
    case VertexComponent::NORMAL:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, normal) });
    case VertexComponent::UV:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexLayout, uv) });
    case VertexComponent::COLOR:
        return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexLayout, color) });
    default:
        return VkVertexInputAttributeDescription({});
    }
}
std::vector<VkVertexInputAttributeDescription> VertexLayout::inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> &components)
{
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t location = 0;
    for (VertexComponent component : components) {
        result.push_back(VertexLayout::inputAttributeDescription(binding, location, component));
        location++;
    }
    return result;
}
void VertexLayout::setPipelineVertexInputState(const std::vector<VertexComponent> &components)
{
    _vertexInputBindingDescription = VkVertexInputBindingDescription({ 0, sizeof(VertexLayout), VK_VERTEX_INPUT_RATE_VERTEX });
    _vertexInputAttributeDescriptions = VertexLayout::inputAttributeDescriptions(0, components);
    _pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &VertexLayout::_vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexLayout::_vertexInputAttributeDescriptions.size()),
        .pVertexAttributeDescriptions = VertexLayout::_vertexInputAttributeDescriptions.data(),
    };
}
}
