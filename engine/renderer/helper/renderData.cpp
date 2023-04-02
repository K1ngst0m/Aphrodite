#include "renderData.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "sceneRenderer.h"

#include "renderer/api/vulkan/device.h"

namespace vkl
{
VulkanRenderData::VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode) :
    m_pDevice{device},
    m_node{std::move(sceneNode)}
{}

VulkanUniformData::VulkanUniformData(VulkanDevice *device, std::shared_ptr<SceneNode> node) :
    m_device{device},
    m_node{std::move(node)}
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
    VK_CHECK_RESULT(m_device->createBuffer(createInfo, &m_buffer, m_object->getData()));
    m_buffer->setupDescriptor();
    m_buffer->map();
}

}  // namespace vkl
