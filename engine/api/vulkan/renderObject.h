#ifndef VKRENDERABLE_H_
#define VKRENDERABLE_H_

#include "scene/sceneRenderer.h"

namespace vkl {

struct MaterialGpuData {
    VkDescriptorSet set;
    VkPipeline      pipeline;
};

class VulkanRenderObject {
public:
    VulkanRenderObject(SceneRenderer *renderer, vkl::VulkanDevice *device, vkl::Entity *entity, VkCommandBuffer drawCmd);
    ~VulkanRenderObject() = default;

    void loadResouces(VkQueue queue);
    void cleanupResources();

    void draw(VkDescriptorSet * globalSet);

    void        setShaderPass(ShaderPass *pass);
    ShaderPass *getShaderPass() const;

    void             setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool);
    uint32_t         getSetCount();

    glm::mat4 getTransform() const;
    void      setTransform(glm::mat4 transform);

private:
    void drawNode(const SubEntity *node);
    void loadImages(VkQueue queue);
    void loadBuffer(vkl::VulkanDevice *device, VkQueue transferQueue, std::vector<VertexLayout> vertices = {}, std::vector<uint32_t> indices = {}, uint32_t vSize = 0, uint32_t iSize = 0);

    vkl::VulkanDevice *_device;
    vkl::ShaderPass   *_shaderPass;

    struct {
        std::vector<VertexLayout> vertices;
        vkl::VulkanBuffer         buffer;
    } _vertexBuffer;

    struct {
        std::vector<uint32_t> indices;
        vkl::VulkanBuffer     buffer;
    } _indexBuffer;

    std::vector<vkl::VulkanTexture> _textures;

    std::vector<MaterialGpuData> _materialGpuDataList;
    const VkCommandBuffer        _drawCmd;

private:
    vkl::SceneRenderer *_renderer;
    glm::mat4           _transform;
    vkl::Entity        *_entity;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
