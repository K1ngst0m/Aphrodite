#include "mesh.h"
#include "scene/camera.h"

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
void Mesh::setup(vkl::Device* device, VkQueue transferQueue, std::vector<VertexLayout> vertices, std::vector<uint32_t> indices, uint32_t vSize, uint32_t iSize)
{
    if(!vertices.empty()){
        _vertices = std::move(vertices);
    }
    if(!indices.empty()){
        _indices = std::move(indices);
    }

    assert(!_vertices.empty());

    if (getIndicesCount() == 0) {
        for (size_t i = 0; i < getVerticesCount(); i++) {
            _indices.push_back(i);
        }
    }

    // setup vertex buffer
    {
        VkDeviceSize bufferSize = vSize == 0 ? sizeof(_vertices[0]) * getVerticesCount() : vSize;
        // using staging buffer
        if (transferQueue != VK_NULL_HANDLE) {
            vkl::Buffer stagingBuffer;
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(_vertices.data(), static_cast<VkDeviceSize>(bufferSize));
            stagingBuffer.unmap();

            device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);
            device->copyBuffer(transferQueue, stagingBuffer, vertexBuffer, bufferSize);

            stagingBuffer.destroy();
        }

        else {
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer);
            vertexBuffer.map();
            vertexBuffer.copyTo(_vertices.data(), static_cast<VkDeviceSize>(bufferSize));
            vertexBuffer.unmap();
        }
    }

    // setup index buffer
    {
        VkDeviceSize bufferSize = iSize == 0 ? sizeof(_indices[0]) * getIndicesCount() : iSize;
        // using staging buffer
        if (transferQueue != VK_NULL_HANDLE) {
            vkl::Buffer stagingBuffer;
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(_indices.data(), static_cast<VkDeviceSize>(bufferSize));
            stagingBuffer.unmap();

            device->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);
            device->copyBuffer(transferQueue, stagingBuffer, indexBuffer, bufferSize);

            stagingBuffer.destroy();
        }

        else {
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexBuffer);
            indexBuffer.map();
            indexBuffer.copyTo(_indices.data(), static_cast<VkDeviceSize>(bufferSize));
            indexBuffer.unmap();
        }
    }
}
void Mesh::destroy() const {
    vertexBuffer.destroy();
    indexBuffer.destroy();
}
uint32_t Mesh::getIndicesCount() const {
    return _indices.size();
}
uint32_t Mesh::getVerticesCount() const {
    return _vertices.size();
}
VkBuffer Mesh::getIndexBuffer() const {
    return indexBuffer.buffer;
}
VkBuffer Mesh::getVertexBuffer() const {
    return vertexBuffer.buffer;
}
} // namespace vkl
