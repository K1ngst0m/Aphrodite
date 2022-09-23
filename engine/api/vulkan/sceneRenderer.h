#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "renderObject.h"
#include "uniformBufferObject.h"

namespace vkl {

class VulkanSceneRenderer : public SceneRenderer {
public:
    VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::VulkanDevice *device, VkQueue graphics, VkQueue transfer);
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
    vkl::VulkanDevice     *_device;
    vkl::ShaderPass *_pass;
    VkCommandBuffer  _drawCmd;
    VkDescriptorPool _descriptorPool;
    VkQueue          _transferQueue;
    VkQueue          _graphicsQueue;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>>       _renderList;
    std::deque<std::unique_ptr<VulkanUniformBufferObject>> _uboList;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
