#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "pipeline.h"
#include "renderer/sceneRenderer.h"

namespace vkl {
class VulkanUniformObject;
class VulkanRenderObject;
class VulkanRenderer;

enum DescriptorSetBinding {
    SET_BINDING_SCENE    = 0,
    SET_BINDING_MATERIAL = 1,
};

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
    void _initUniformList();

    void _setupUnlitShaderEffect();
    void _setupDefaultLitShaderEffect();

    void _loadSceneNodes(std::unique_ptr<SceneNode> &node);

private:
    VkDescriptorSetLayout       *_getDescriptorSetLayout(DescriptorSetBinding binding);
    std::unique_ptr<ShaderPass> &_getShaderPass();

private:
    VkDescriptorSet  _globalDescriptorSet;
    VkDescriptorPool _descriptorPool;
    ShaderCache      _shaderCache;

    std::unique_ptr<ShaderEffect> _unlitEffect = nullptr;
    std::unique_ptr<ShaderPass>   _unlitPass   = nullptr;

    std::unique_ptr<ShaderEffect> _defaultLitEffect = nullptr;
    std::unique_ptr<ShaderPass>   _defaultLitPass   = nullptr;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>> _renderList;
    std::deque<std::unique_ptr<VulkanUniformObject>> _uniformList;

private:
    std::shared_ptr<VulkanDevice> _device;
    VulkanRenderer               *_renderer;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
