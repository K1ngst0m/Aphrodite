#ifndef VKRENDERABLE_H_
#define VKRENDERABLE_H_

#include "scene/sceneRenderer.h"

namespace vkl {
class VulkanRenderable : public Renderable{
public:
    VulkanRenderable(SceneRenderer *renderer, vkl::Device *device, vkl::Entity *entity, VkCommandBuffer drawCmd);
    ~VulkanRenderable() override = default;

    void loadResouces(VkQueue queue);
    void cleanupResources();

    std::vector<VkDescriptorPoolSize> getDescriptorSetInfo() const;
    void setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool);

    void draw() override;

    void setShaderPass(ShaderPass* pass){
        _shaderPass = pass;
    }

    ShaderPass * getShaderPass() const{
        return _shaderPass;
    }

    VkDescriptorSet& getGlobalSet(){
        return globalDescriptorSet;
    }

private:
    void drawNode(const Entity::Node *node);
    void loadImages(VkQueue queue);
    vkl::Texture *getTexture(uint32_t index);

    vkl::Device                 *_device;
    vkl::ShaderPass             *_shaderPass;

    vkl::Mesh                    _mesh;
    std::vector<vkl::Texture>    _textures;

    std::vector<VkDescriptorSet> materialSets;
    VkDescriptorSet              globalDescriptorSet;
    const VkCommandBuffer drawCmd;
};
}

#endif // VKRENDERABLE_H_
