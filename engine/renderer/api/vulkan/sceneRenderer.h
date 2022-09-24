#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "scene/sceneRenderer.h"

namespace vkl {
class VulkanUniformBufferObject;
class VulkanRenderObject;
class VulkanDevice;

class VulkanSceneRenderer : public SceneRenderer {
public:
    VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, VulkanDevice *device, VkQueue graphics, VkQueue transfer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update() override;
    void drawScene() override;
    void setShaderPass(vkl::ShaderPass *pass);

private:
    void _initRenderList();
    void _initUboList();
    void _loadSceneNodes(SceneNode *node);

private:
    vkl::VulkanDevice *_device;
    vkl::ShaderPass   *_pass;
    VkCommandBuffer    _drawCmd;
    VkDescriptorPool   _descriptorPool;
    VkDescriptorSet    _globalDescriptorSet;
    VkQueue            _transferQueue;
    VkQueue            _graphicsQueue;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>>       _renderList;
    std::deque<std::unique_ptr<VulkanUniformBufferObject>> _uboList;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
