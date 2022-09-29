#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "device.h"
#include "buffer.h"
#include "texture.h"

namespace vkl {
class VulkanSceneRenderer;
class Entity;
class ShaderPass;
struct Vertex;
struct SubEntity;

struct MaterialGpuData {
    VkDescriptorSet set;
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

    void loadResouces(VkQueue queue);
    void cleanupResources();

    void draw(VkPipelineLayout layout, VkCommandBuffer drawCmd);

    void     setupMaterial(VkDescriptorSetLayout *materialLayout, VkDescriptorPool descriptorPool, uint8_t bindingBits);
    uint32_t getSetCount();

    glm::mat4 getTransform() const;
    void      setTransform(glm::mat4 transform);

private:
    void drawNode(VkPipelineLayout layout, VkCommandBuffer drawCmd, const std::shared_ptr<SubEntity> &node);
    void loadTextures(VkQueue queue);
    void loadBuffer(VkQueue transferQueue);
    void createEmptyTexture(VkQueue queue);

    std::shared_ptr<VulkanDevice> _device = nullptr;

    struct {
        std::vector<Vertex> vertices;
        VulkanBuffer        buffer;
    } _vertexBuffer;

    struct {
        std::vector<uint32_t> indices;
        VulkanBuffer          buffer;
    } _indexBuffer;

    VulkanTexture                _emptyTexture;
    std::vector<VulkanTexture>   _textures;
    std::vector<MaterialGpuData> _materialGpuDataList;

private:
    VulkanSceneRenderer *_renderer;
    glm::mat4            _transform;
    Entity         *_entity;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
