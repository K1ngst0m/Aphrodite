#ifndef VKLSCENERENDERER_H_
#define VKLSCENERENDERER_H_

#include "vklDevice.h"
#include "vklSceneManger.h"

namespace vkl {
class SceneRenderer {
public:
    SceneRenderer(Scene *scene)
        :_scene(scene)
    {}

    virtual void prepareResource() = 0;
    virtual void drawScene()    = 0;

    virtual void destroy() = 0;

    void setScene(Scene *scene);

protected:
    Scene *_scene;
};

class VulkanSceneRenderer final : public SceneRenderer {
public:
    VulkanSceneRenderer(Scene *scene, VkCommandBuffer commandBuffer, vkl::Device *device);

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
        vkl::Entity * object;
        glm::mat4 transform;

        vkl::ShaderPass * shaderPass;

        void draw(VkCommandBuffer commandBuffer) const{
            object->draw(commandBuffer, shaderPass, transform);
        }
    };

    std::vector<Renderable> _renderList;

    VkDescriptorPool _descriptorPool;
};
} // namespace vkl

#endif // VKLSCENERENDERER_H_
