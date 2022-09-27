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

    std::unique_ptr<ShaderPass> &getPrebuiltPass(ShadingModel type);

private:
    void _initRenderResource();
    void _initRenderList();
    void _setupBaseColorShaderEffect();
    void _setupPBRShaderEffect();
    void _initUboList();
    void _loadSceneNodes(std::unique_ptr<SceneNode> &node);

private:
    VkDescriptorPool _descriptorPool;
    vkl::ShaderCache m_shaderCache;

    std::unique_ptr<vkl::ShaderEffect> _unlitEffect = nullptr;
    std::unique_ptr<vkl::ShaderPass>   _unlitPass   = nullptr;

    std::unique_ptr<vkl::ShaderEffect> _defaultLitEffect = nullptr;
    std::unique_ptr<vkl::ShaderPass>   _defaultLitPass   = nullptr;

private:
    std::vector<std::unique_ptr<VulkanRenderObject>> _renderList;
    std::deque<std::unique_ptr<VulkanUniformObject>> _uboList;

private:
    vkl::VulkanDevice *_device;
    VulkanRenderer    *_renderer;
    VkQueue            _transferQueue;
    VkQueue            _graphicsQueue;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
