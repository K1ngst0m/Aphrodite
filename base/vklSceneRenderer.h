#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "vklDevice.h"
#include "vklSceneManger.h"

namespace vkl {
class SceneRenderer {
public:
    SceneRenderer(SceneManager *scene)
        :_scene(scene)
    {}

    virtual void prepareResource() = 0;
    virtual void drawScene()    = 0;
    virtual void destroy() = 0;

    void setScene(SceneManager *scene);

protected:
    SceneManager *_scene;
};

class VulkanSceneRenderer final : public SceneRenderer {
public:
    VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::Device *device);

    void prepareResource() override;
    void drawScene() override;

    void destroy() override;

private:
    void _initRenderList();
    void _setupDescriptor();

private:
    VkCommandBuffer _drawCmd;
    vkl::Device    *_device;

    struct Renderable{
        VkDescriptorSet globalDescriptorSet;
        std::vector<VkDescriptorSet> materialSet;
        vkl::Entity * entity;
        glm::mat4 transform;
        vkl::ShaderPass * shaderPass;

        void draw(VkCommandBuffer commandBuffer) const{
            entity->draw(commandBuffer, shaderPass, transform);
        }

        // void setupDescriptor(vkl::Device * device, VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) {
        //     for (auto &set : materialSet) {
        //         VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(descriptorPool, &layout, 1);
        //         VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &set));
        //         VkWriteDescriptorSet writeDescriptorSet = vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &material.baseColorTexture->descriptorInfo);
        //         vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
        //     }
        // }
    };

    std::vector<Renderable> _renderList;

    VkDescriptorPool _descriptorPool;
};
} // namespace vkl

#endif // VKLSCENERENDERER_H_
