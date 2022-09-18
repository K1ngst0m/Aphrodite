#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "vklDevice.h"
#include "vklSceneManger.h"

namespace vkl {
class SceneRenderer;
struct Renderable{
    Renderable(SceneRenderer * renderer, vkl::Entity* entity)
        : _renderer(renderer), entity(entity)
    {}

    virtual void draw() = 0;

    glm::mat4 transform;

    vkl::SceneRenderer * _renderer;
    vkl::Entity * entity;
};

class SceneRenderer {
public:
    SceneRenderer(SceneManager *sceneManager);

    virtual void prepareResource() = 0;
    virtual void drawScene() = 0;

    virtual void destroy() = 0;

    void setScene(SceneManager *scene);

protected:
    SceneManager *_sceneManager;
};

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
} // namespace vkl

#endif // VKLSCENERENDERER_H_
