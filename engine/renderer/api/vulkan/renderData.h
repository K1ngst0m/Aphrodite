#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "sceneRenderer.h"

namespace vkl
{

struct TextureGpuData
{
    VulkanImage *image = nullptr;
    VulkanImageView *imageView = nullptr;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorImageInfo descriptorInfo;

    void setupDescriptor();
};

struct MaterialGpuData
{
    VulkanBuffer *buffer = nullptr;
    VkDescriptorSet set = VK_NULL_HANDLE;
};

struct VulkanRenderData
{
    VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode);
    ~VulkanRenderData();

    void setupDescriptor(VulkanDescriptorSetLayout *objectLayout,
                         VulkanDescriptorSetLayout *materialLayout, uint8_t bindingBits);
    uint32_t getSetCount();

    VulkanBuffer *m_vertexBuffer = nullptr;
    VulkanBuffer *m_indexBuffer = nullptr;

    VulkanBuffer *m_objectUB = nullptr;
    VkDescriptorSet m_objectSet = VK_NULL_HANDLE;

    std::vector<TextureGpuData> m_textures;
    std::vector<MaterialGpuData> m_materialGpuDataList;

    VulkanDevice *m_pDevice = nullptr;
    std::shared_ptr<SceneNode> m_node = nullptr;
};

struct VulkanUniformData {
    VulkanUniformData(VulkanDevice *device, std::shared_ptr<SceneNode> node);
    ~VulkanUniformData();

    VulkanBuffer *m_buffer = nullptr;
    VulkanDevice *m_device = nullptr;

    std::shared_ptr<SceneNode>     m_node = nullptr;
    std::shared_ptr<UniformObject> m_object  = nullptr;
};
}  // namespace vkl

#endif  // VKRENDERABLE_H_
