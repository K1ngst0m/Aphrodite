#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "scene/sceneRenderer.h"

namespace vkl {
struct VulkanRenderable : Renderable{
    VulkanRenderable(SceneRenderer *renderer, vkl::Device *device, vkl::Entity *entity, VkCommandBuffer drawCmd);

    void draw() override;
    void drawNode(const Entity::Node *node);

    void loadResouces(VkQueue queue);
    void setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool);

    std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() const;

    void loadImages(VkQueue queue);

    vkl::Texture *getTexture(uint32_t index);

    vkl::Device                 *_device;
    vkl::ShaderPass             *shaderPass;
    std::vector<VkDescriptorSet> materialSet;
    VkDescriptorSet              globalDescriptorSet;

    // device data
    std::vector<vkl::Texture>    _textures;
    std::vector<VkDescriptorSet> materialSets;
    vkl::Mesh                    _mesh;

    const VkCommandBuffer drawCmd;
};

class VulkanSceneRenderer : public SceneRenderer {
public:
    VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::Device *device, VkQueue graphics, VkQueue transfer);
    void prepareResource() override;
    void destroy() override;
    void drawScene() override;

private:
    void _initRenderList();
    void _setupDescriptor();

private:
    vkl::Device    *_device;
    VkCommandBuffer _drawCmd;
    VkDescriptorPool _descriptorPool;

    VkQueue _transferQueue;
    VkQueue _graphicsQueue;

    std::vector<VulkanRenderable*> _renderList;
};
}

#endif // VKSCENERENDERER_H_
