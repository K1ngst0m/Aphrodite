#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "device.h"
#include "renderer/sceneRenderer.h"
#include "vulkanRenderer.h"

namespace vkl {
class ShaderPass;
class VulkanUniformObject;
class VulkanRenderObject;
class VulkanRenderer;

struct MaterialGpuData {
    VkDescriptorSet set;
    VkPipeline      pipeline;
};

struct TextureGpuData {
    VulkanImage     *image     = nullptr;
    VulkanImageView *imageView = nullptr;
    VkSampler        sampler   = VK_NULL_HANDLE;

    VkDescriptorImageInfo descriptorInfo;
};

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
    static std::unique_ptr<VulkanSceneRenderer> Create(VulkanRenderer *renderer);
    VulkanSceneRenderer(VulkanRenderer *renderer);
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
    VkDescriptorSetLayout *_getDescriptorSetLayout(DescriptorSetBinding binding);
    VulkanPipeline        *_getCurrentPipeline();

private:
    std::vector<VkDescriptorSet> _globalDescriptorSets;
    VkDescriptorPool             _descriptorPool;

    ShaderEffect   *_unlitEffect   = nullptr;
    VulkanPipeline *_unlitPipeline = nullptr;

    ShaderEffect   *_defaultLitEffect   = nullptr;
    VulkanPipeline *_defaultLitPipeline = nullptr;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>> _renderList;
    std::deque<std::unique_ptr<VulkanUniformObject>> _uniformList;

private:
    VulkanDevice   *_device;
    VulkanRenderer *_renderer;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
