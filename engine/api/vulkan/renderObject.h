#ifndef VKRENDERABLE_H_
#define VKRENDERABLE_H_

#include "scene/sceneRenderer.h"

namespace vkl {
class VulkanRenderObject : public RenderObject {
public:
    VulkanRenderObject(SceneRenderer *renderer, vkl::Device *device, vkl::Entity *entity, VkCommandBuffer drawCmd);
    ~VulkanRenderObject() override {
        cleanupResources();
    }

    void loadResouces(VkQueue queue);
    void cleanupResources();

    void draw() override;

    void        setShaderPass(ShaderPass *pass);
    ShaderPass *getShaderPass() const;

    void                              setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool);
    VkDescriptorSet                  &getGlobalDescriptorSet();
    uint32_t                          getSetCount();

private:
    void          drawNode(const Entity::Node *node);
    void          loadImages(VkQueue queue);
    vkl::Texture *getTexture(uint32_t index);

    vkl::Device     *_device;
    vkl::ShaderPass *_shaderPass;

    vkl::Mesh                 _mesh;
    std::vector<vkl::Texture> _textures;

    std::vector<VkDescriptorSet> _materialSets;
    VkDescriptorSet              _globalDescriptorSet;
    const VkCommandBuffer        _drawCmd;
};
} // namespace vkl

#endif // VKRENDERABLE_H_
