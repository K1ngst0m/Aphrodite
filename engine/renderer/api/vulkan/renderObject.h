#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "device.h"

namespace vkl {
class VulkanSceneRenderer;
class Entity;
class ShaderPass;
struct Vertex;
struct SubEntity;
class VulkanImage;
class VulkanImageView;
class VulkanSampler;

struct MaterialGpuData {
    VkDescriptorSet set;
};

struct TextureGpuData {
    VulkanImage     *image;
    VulkanImageView *imageView;
    VkSampler        sampler;

    VkDescriptorImageInfo descriptorInfo;
};

enum MaterialBindingBits {
    MATERIAL_BINDING_NONE      = (1 << 0),
    MATERIAL_BINDING_BASECOLOR = (1 << 1),
    MATERIAL_BINDING_NORMAL    = (1 << 2),
};

class VulkanRenderObject {
public:
    VulkanRenderObject(VulkanSceneRenderer *renderer, const std::shared_ptr<VulkanDevice> &device, Entity *entity);
    ~VulkanRenderObject() = default;

    void loadResouces();
    void cleanupResources();

    void draw(VkPipelineLayout layout, VkCommandBuffer drawCmd);

    void     setupMaterial(VkDescriptorSetLayout *materialLayout, VkDescriptorPool descriptorPool, uint8_t bindingBits);
    uint32_t getSetCount();

    glm::mat4 getTransform() const;
    void      setTransform(glm::mat4 transform);

private:
    void drawNode(VkPipelineLayout layout, VkCommandBuffer drawCmd, const std::shared_ptr<SubEntity> &node);
    void loadTextures();
    void loadBuffer();
    void createEmptyTexture();

    std::shared_ptr<VulkanDevice> _device = nullptr;

    struct {
        std::vector<Vertex> vertices;
        VulkanBuffer       *buffer;
    } _vertexBuffer;

    struct {
        std::vector<uint32_t> indices;
        VulkanBuffer         *buffer;
    } _indexBuffer;

    TextureGpuData               _emptyTexture;
    std::vector<TextureGpuData>  _textures;
    std::vector<MaterialGpuData> _materialGpuDataList;

private:
    VulkanSceneRenderer *_renderer;
    glm::mat4            _transform;
    Entity              *_entity;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
