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
VulkanRenderData::VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode) :
    m_pDevice(device),
    m_node(std::move(sceneNode))
{
    std::vector<Vertex> vertices;
    std::vector<uint8_t> indices;

    {
        auto mesh = m_node->getObject<Mesh>();

        {
            vertices = mesh->m_vertices;
            indices = mesh->m_indices;
        }
    }

    assert(!vertices.empty());

    // load buffer
    // setup vertex buffer
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        // using staging buffer
        vkl::VulkanBuffer *stagingBuffer;
        {
            BufferCreateInfo createInfo{};
            createInfo.size = bufferSize;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            createInfo.usage = BUFFER_USAGE_TRANSFER_SRC_BIT;
            m_pDevice->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(vertices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage = BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            m_pDevice->createBuffer(&createInfo, &m_vertexBuffer);
        }

        auto cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
        m_pDevice->endSingleTimeCommands(cmd);

        m_pDevice->destroyBuffer(stagingBuffer);
    }

    // setup index buffer
    if(!indices.empty())
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        // using staging buffer
        vkl::VulkanBuffer *stagingBuffer = nullptr;

        {
            BufferCreateInfo createInfo{};
            createInfo.size = bufferSize;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            createInfo.usage = BUFFER_USAGE_TRANSFER_SRC_BIT;
            m_pDevice->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(indices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage = BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            m_pDevice->createBuffer(&createInfo, &m_indexBuffer);
        }

        auto cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
        m_pDevice->endSingleTimeCommands(cmd);

        m_pDevice->destroyBuffer(stagingBuffer);
    }
}

VulkanRenderData::~VulkanRenderData()
{
    if(m_indexBuffer)
    {
        m_pDevice->destroyBuffer(m_indexBuffer);
    }
    m_pDevice->destroyBuffer(m_vertexBuffer);
    m_pDevice->destroyBuffer(m_objectUB);
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

VulkanUniformData::~VulkanUniformData()
{
    m_device->destroyBuffer(m_buffer);
};

}  // namespace vkl
