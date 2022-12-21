#include "renderData.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "descriptorSetLayout.h"
#include "device.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "scene/camera.h"
#include "scene/mesh.h"
#include "scene/light.h"
#include "sceneRenderer.h"
#include "vkInit.hpp"

namespace vkl
{
namespace {
VulkanBuffer * createBuffer(VulkanDevice * pDevice, const void* data, VkDeviceSize size, VkBufferUsageFlags usage){
    VulkanBuffer * buffer = nullptr;
    // setup vertex buffer
    {
        VkDeviceSize bufferSize = size;
        // using staging buffer
        vkl::VulkanBuffer *stagingBuffer;
        {
            BufferCreateInfo createInfo{};
            createInfo.size = bufferSize;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            createInfo.usage = BUFFER_USAGE_TRANSFER_SRC_BIT;
            pDevice->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(data, static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            pDevice->createBuffer(&createInfo, &buffer);
        }

        auto cmd = pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, buffer, bufferSize);
        pDevice->endSingleTimeCommands(cmd);

        pDevice->destroyBuffer(stagingBuffer);
    }
    return buffer;
}
}

VulkanRenderData::VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode) :
    m_pDevice(device),
    m_node(std::move(sceneNode))
{
    auto mesh = m_node->getObject<Mesh>();

    auto &vertices = mesh->m_vertices;
    auto &indices = mesh->m_indices;

    // load buffer
    assert(!vertices.empty());
    m_vertexBuffer = createBuffer(m_pDevice, vertices.data(), sizeof(vertices[0]) * vertices.size(), BUFFER_USAGE_VERTEX_BUFFER_BIT);
    if (!indices.empty()){
        m_indexBuffer = createBuffer(m_pDevice, indices.data(), sizeof(indices[0]) * indices.size(), BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
}

VulkanUniformData::VulkanUniformData(VulkanDevice *device, std::shared_ptr<SceneNode> node) :
    m_device(device),
    m_node(std::move(node))
{
    switch(m_node->m_attachType)
    {
    case ObjectType::LIGHT:
        m_object = m_node->getObject<Light>();
    case ObjectType::CAMERA:
        m_object = m_node->getObject<Camera>();
    default:
        assert("invalid object type");
    }
    m_object->load();

    BufferCreateInfo createInfo{};
    createInfo.size = m_object->getDataSize();
    createInfo.usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VK_CHECK_RESULT(m_device->createBuffer(&createInfo, &m_buffer, m_object->getData()));
    m_buffer->setupDescriptor();
    m_buffer->map();
}

}  // namespace vkl
