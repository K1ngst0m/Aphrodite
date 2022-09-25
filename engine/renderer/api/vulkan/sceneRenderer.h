#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "pipeline.h"
#include "renderer/sceneRenderer.h"

namespace vkl {
class VulkanUniformBufferObject;
class VulkanRenderObject;
class VulkanRenderer;

class VulkanSceneRenderer : public SceneRenderer {
public:
    VulkanSceneRenderer(VulkanRenderer *renderer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update() override;
    void drawScene() override;

private:
    void _initRenderResource();
    void _initRenderList();
    void _setupDefaultShaderEffect();
    void _initUboList();
    void _loadSceneNodes(SceneNode *node);

private:
    VkDescriptorPool                   _descriptorPool;
    std::vector<VkDescriptorSet>       _globalDescriptorSets;
    vkl::ShaderCache                   m_shaderCache;
    std::unique_ptr<vkl::ShaderEffect> _effect;
    std::unique_ptr<vkl::ShaderPass>   _pass;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>>       _renderList;
    std::deque<std::unique_ptr<VulkanUniformBufferObject>> _uboList;

private:
    vkl::VulkanDevice *_device;
    VulkanRenderer    *_renderer;
    VkQueue            _transferQueue;
    VkQueue            _graphicsQueue;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
