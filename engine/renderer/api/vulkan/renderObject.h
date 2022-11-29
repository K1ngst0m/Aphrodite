#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "sceneRenderer.h"

namespace vkl {

struct TextureGpuData {
    VulkanImage     *image     = nullptr;
    VulkanImageView *imageView = nullptr;
    VkSampler        sampler   = VK_NULL_HANDLE;

    VkDescriptorImageInfo descriptorInfo;
};

struct VulkanMeshData {
    VulkanBuffer *vb = nullptr;
    VulkanBuffer *ib  = nullptr;
};

class VulkanRenderData {
public:
    VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode);
    ~VulkanRenderData() = default;

    void loadResouces();
    void cleanupResources();

    void draw(VulkanPipeline *pipeline, VulkanCommandBuffer *drawCmd);

    void     setupDescriptor(VulkanDescriptorSetLayout* objectLayout, VulkanDescriptorSetLayout *materialLayout, uint8_t bindingBits);
    uint32_t getSetCount();

private:
    void loadTextures();
    void loadBuffer();

    VulkanMeshData               _meshData;
    TextureGpuData               _emptyTexture;
    std::vector<TextureGpuData>  _textures;
    std::vector<VkDescriptorSet> _materialSets;

    VulkanBuffer *               _objectUB = nullptr;
    VkDescriptorSet              _objectSet = VK_NULL_HANDLE;

    TextureGpuData createTexture(uint32_t width, uint32_t height, void *data, uint32_t dataSize);

private:
    VulkanDevice           *_device    = nullptr;
    public:
    std::shared_ptr<SceneNode> _node = nullptr;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
