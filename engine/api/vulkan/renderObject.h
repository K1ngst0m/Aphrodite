#ifndef VKRENDERABLE_H_
#define VKRENDERABLE_H_

#include "scene/sceneRenderer.h"

namespace vkl {

struct MaterialGpuData{
    VkDescriptorSet set;
    VkPipeline pipeline;
};

class VulkanRenderObject : public RenderObject {
public:
    VulkanRenderObject(SceneRenderer *renderer, vkl::VulkanDevice *device, vkl::Entity *entity, VkCommandBuffer drawCmd);
    ~VulkanRenderObject() override {
        cleanupResources();
    }

    void loadResouces(VkQueue queue);
    void cleanupResources();

    void draw() override;

    void        setShaderPass(ShaderPass *pass);
    ShaderPass *getShaderPass() const;

    void             setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool);
    VkDescriptorSet &getGlobalDescriptorSet();
    uint32_t         getSetCount();

private:
    void drawNode(const SubEntity *node);
    void loadImages(VkQueue queue);
    void loadBuffer(vkl::VulkanDevice *device, VkQueue transferQueue, std::vector<VertexLayout> vertices = {}, std::vector<uint32_t> indices = {}, uint32_t vSize = 0, uint32_t iSize = 0);

    vkl::VulkanDevice     *_device;
    vkl::ShaderPass *_shaderPass;

    struct VulkanVertexBuffer {
        std::vector<VertexLayout> vertices;
        vkl::VulkanBuffer         buffer;
    };

    VulkanVertexBuffer _vertexBuffer;

    struct VulkanIndexBuffer {
        std::vector<uint32_t> indices;
        vkl::VulkanBuffer     buffer;
    };

    VulkanIndexBuffer _indexBuffer;

    std::vector<vkl::VulkanTexture> _textures;


    std::vector<MaterialGpuData> _materialGpuDataList;
    VkDescriptorSet              _globalDescriptorSet;
    const VkCommandBuffer        _drawCmd;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
