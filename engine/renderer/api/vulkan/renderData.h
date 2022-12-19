#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "sceneRenderer.h"

namespace vkl
{
struct VulkanRenderData
{
    VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode);
    ~VulkanRenderData();

    VulkanBuffer *m_vertexBuffer = nullptr;
    VulkanBuffer *m_indexBuffer = nullptr;

    VulkanBuffer *m_objectUB = nullptr;
    VkDescriptorSet m_objectSet = VK_NULL_HANDLE;

    VulkanDevice *m_pDevice = nullptr;
    std::shared_ptr<SceneNode> m_node = nullptr;
};

struct VulkanUniformData
{
    VulkanUniformData(VulkanDevice *device, std::shared_ptr<SceneNode> node);
    ~VulkanUniformData();

    VulkanBuffer *m_buffer = nullptr;
    VulkanDevice *m_device = nullptr;

    std::shared_ptr<SceneNode> m_node = nullptr;
    std::shared_ptr<UniformObject> m_object = nullptr;
};
}  // namespace vkl

#endif  // VKRENDERABLE_H_
