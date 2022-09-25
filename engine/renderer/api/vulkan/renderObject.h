#ifndef VKRENDERABLE_H_
#define VKRENDERABLE_H_

#include "device.h"

namespace vkl {
class VulkanSceneRenderer;
class Entity;
class ShaderPass;

struct VertexLayout;
struct SubEntity;

struct MaterialGpuData {
    VkDescriptorSet set;
    VkPipeline      pipeline;
};

class VulkanRenderObject {
public:
    VulkanRenderObject(VulkanSceneRenderer *renderer, VulkanDevice *device, Entity *entity);
    ~VulkanRenderObject() = default;

    void loadResouces(VkQueue queue);
    void cleanupResources();

    void draw(VkCommandBuffer drawCmd, VkDescriptorSet * globalSet);

    void        setShaderPass(ShaderPass *pass);
    ShaderPass *getShaderPass() const;

    void             setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool);
    uint32_t         getSetCount();

    glm::mat4 getTransform() const;
    void      setTransform(glm::mat4 transform);

private:
    void drawNode(VkCommandBuffer drawCmd, const SubEntity *node);
    void loadImages(VkQueue queue);
    void loadBuffer(vkl::VulkanDevice *device, VkQueue transferQueue, std::vector<VertexLayout> vertices = {}, std::vector<uint32_t> indices = {}, uint32_t vSize = 0, uint32_t iSize = 0);

    VulkanDevice *_device;
    ShaderPass   *_shaderPass;

    struct {
        std::vector<VertexLayout> vertices;
        VulkanBuffer         buffer;
    } _vertexBuffer;

    struct {
        std::vector<uint32_t> indices;
        VulkanBuffer     buffer;
    } _indexBuffer;

    std::vector<VulkanTexture> _textures;

    std::vector<MaterialGpuData> _materialGpuDataList;

private:
    VulkanSceneRenderer *_renderer;
    glm::mat4           _transform;
    vkl::Entity        *_entity;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
