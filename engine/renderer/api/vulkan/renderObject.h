#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "sceneRenderer.h"

namespace vkl {

struct MaterialGpuData {
    VkDescriptorSet set;
    VkPipeline      pipeline;
};

struct TextureGpuData {
    VulkanImage     *image     = nullptr;
    VulkanImageView *imageView = nullptr;
    VkSampler        sampler   = VK_NULL_HANDLE;

    VkDescriptorImageInfo descriptorInfo;
};

struct VulkanMeshData {
    VulkanBuffer *_vertexBuffer = nullptr;
    VulkanBuffer *_indexBuffer  = nullptr;
};

class VulkanRenderData {
public:
    VulkanRenderData(VulkanDevice *device, std::shared_ptr<Entity> entity);
    ~VulkanRenderData() = default;

    void loadResouces();
    void cleanupResources();

    void draw(VulkanPipeline *pipeline, VulkanCommandBuffer *drawCmd);

    void     setupMaterial(VulkanDescriptorSetLayout *materialLayout, uint8_t bindingBits);
    uint32_t getSetCount();

    glm::mat4 getTransform() const;
    void      setTransform(glm::mat4 transform);

private:
    void loadTextures();
    void loadBuffer();

    VulkanMeshData               _meshData;
    TextureGpuData               _emptyTexture;
    std::vector<TextureGpuData>  _textures;
    std::vector<MaterialGpuData> _materialGpuDataList;

    TextureGpuData createTexture(uint32_t width, uint32_t height, void *data, uint32_t dataSize);

private:
    VulkanDevice           *_device    = nullptr;
    std::shared_ptr<Entity> _entity    = nullptr;
    glm::mat4               _transform = glm::mat4(1.0f);
};
} // namespace vkl

#endif // VKRENDERABLE_H_
