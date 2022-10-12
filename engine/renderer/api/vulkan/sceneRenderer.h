#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "pipeline.h"
#include "renderer/api/vulkan/shader.h"
#include "renderer/sceneRenderer.h"

namespace vkl {
class ShaderPass;
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
    void updateScene() override;
    void drawScene() override;

private:
    void _initRenderList();
    void _initUniformList();

    void _setupUnlitShaderEffect();
    void _setupDefaultLitShaderEffect();

    void _loadSceneNodes(std::unique_ptr<SceneNode> &node);

private:
    VkDescriptorSetLayout *_getDescriptorSetLayout(DescriptorSetBinding binding);
    VulkanPipeline        *_getCurrentPipeline();

private:
    std::vector<VkDescriptorSet> _globalDescriptorSets;
    VkDescriptorPool             _descriptorPool;

    std::shared_ptr<ShaderEffect> _unlitEffect   = nullptr;
    VulkanPipeline               *_unlitPipeline = nullptr;

    std::shared_ptr<ShaderEffect> _defaultLitEffect   = nullptr;
    VulkanPipeline               *_defaultLitPipeline = nullptr;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>> _renderList;
    std::deque<std::unique_ptr<VulkanUniformObject>> _uniformList;

private:
    VulkanDevice   *_device;
    VulkanRenderer *_renderer;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
