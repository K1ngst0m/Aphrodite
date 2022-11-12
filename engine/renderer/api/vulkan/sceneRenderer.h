#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "device.h"
#include "renderer/sceneRenderer.h"
#include "vulkanRenderer.h"

namespace vkl {
class ShaderPass;
class VulkanUniformData;
class VulkanRenderData;
class VulkanRenderer;


enum MaterialBindingBits {
    MATERIAL_BINDING_NONE      = (1 << 0),
    MATERIAL_BINDING_BASECOLOR = (1 << 1),
    MATERIAL_BINDING_NORMAL    = (1 << 2),
};

enum DescriptorSetBinding {
    SET_BINDING_SCENE    = 0,
    SET_BINDING_MATERIAL = 1,
};

class VulkanSceneRenderer : public SceneRenderer {
public:
    static std::unique_ptr<VulkanSceneRenderer> Create(const std::shared_ptr<VulkanRenderer>& renderer);
    VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer>& renderer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update(float deltaTime) override;
    void drawScene() override;

private:
    void _initRenderList();
    void _initUniformList();

    void _setupUnlitShaderEffect();
    void _setupDefaultLitShaderEffect();

    void _loadSceneNodes(std::unique_ptr<SceneNode> &node);

private:
    VulkanPipeline            *_getCurrentPipeline();

private:
    std::vector<VkDescriptorSet> _globalDescriptorSets;

    VulkanPipeline *_unlitPipeline = nullptr;
    VulkanPipeline *_defaultLitPipeline = nullptr;

private:
    std::vector<std::unique_ptr<VulkanRenderData>> _renderList;
    std::deque<std::unique_ptr<VulkanUniformData>> _uniformList;

private:
    VulkanDevice   *_device;
    std::shared_ptr<VulkanRenderer> _renderer;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
