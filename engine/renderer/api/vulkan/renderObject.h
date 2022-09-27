#ifndef VULKAN_RENDERABLE_H_
#define VULKAN_RENDERABLE_H_

#include "device.h"
#include "uniformObject.h"

namespace vkl {
class VulkanSceneRenderer;
class Entity;
class ShaderPass;

struct Vertex;
struct SubEntity;

struct MaterialGpuData {
    VkDescriptorSet set;
};

class VulkanRenderObject {
public:
    VulkanRenderObject(VulkanSceneRenderer *renderer, VulkanDevice *device, Entity *entity, std::unique_ptr<ShaderPass> &shaderPass);
    ~VulkanRenderObject() = default;

    void loadResouces(VkQueue queue);
    void cleanupResources();

    void draw(VkCommandBuffer drawCmd);

    void     setupGlobalDescriptorSet(VkDescriptorPool descriptorPool, std::deque<std::unique_ptr<VulkanUniformObject>> &uboList);
    void     setupMaterial(VkDescriptorPool descriptorPool);
    uint32_t getSetCount();

    glm::mat4 getTransform() const;
    void      setTransform(glm::mat4 transform);

private:
    void drawNode(VkCommandBuffer drawCmd, const SubEntity *node);
    void loadTextures(VkQueue queue);
    void loadBuffer(VkQueue transferQueue);
    void createEmptyTexture(VkQueue queue);

    VulkanDevice                *_device;
    std::unique_ptr<ShaderPass> &_shaderPass;

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
    VkDescriptorSet              _globalDescriptorSet;

private:
    VulkanSceneRenderer *_renderer;
    glm::mat4            _transform;
    vkl::Entity         *_entity;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
