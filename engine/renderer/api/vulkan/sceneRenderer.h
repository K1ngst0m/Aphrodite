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
    MATERIAL_BINDING_NONE       = 0,
    MATERIAL_BINDING_BASECOLOR  = (1 << 0),
    MATERIAL_BINDING_NORMAL     = (1 << 1),
    MATERIAL_BINDING_PHYSICAL   = (1 << 2),
    MATERIAL_BINDING_AO         = (1 << 3),
    MATERIAL_BINDING_EMISSIVE   = (1 << 4),
    MATERIAL_BINDING_UNLIT      = MATERIAL_BINDING_BASECOLOR,
    MATERIAL_BINDING_DEFAULTLIT = (MATERIAL_BINDING_BASECOLOR | MATERIAL_BINDING_NORMAL),
    MATERIAL_BINDING_PBR        = (MATERIAL_BINDING_BASECOLOR | MATERIAL_BINDING_NORMAL | MATERIAL_BINDING_PHYSICAL | MATERIAL_BINDING_AO | MATERIAL_BINDING_EMISSIVE),
};

enum DescriptorSetBinding {
    SET_SCENE    = 0,
    SET_OBJECT   = 1,
    SET_MATERIAL = 2,
};

struct SceneInfo {
    glm::vec4 ambient{0.04f};
    uint32_t  cameraCount = 0;
    uint32_t  lightCount  = 0;
};

class VulkanSceneRenderer : public SceneRenderer {
    constexpr static uint32_t SHADOWMAP_DIM    = 2048;
    constexpr static VkFilter SHADOWMAP_FILTER = VK_FILTER_LINEAR;

public:
    static std::unique_ptr<VulkanSceneRenderer> Create(const std::shared_ptr<VulkanRenderer> &renderer);
    VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update(float deltaTime) override;
    void drawScene() override;

private:
    void _initRenderList();
    void _initUniformList();
    void _initSkyboxResource();
    void _initForwardResource();
    void _initPostFxResource();
    void _loadSceneNodes();

private:
    SceneInfo                    _sceneInfo{};
    std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<VulkanBuffer *>  _sceneInfoUBs;

    struct {
        vkl::VulkanRenderPass           *renderPass = nullptr;
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VulkanImage *>       colorImages;
        std::vector<VulkanImageView *>   colorImageViews;

        VulkanImage     *depthImage     = nullptr;
        VulkanImageView *depthImageView = nullptr;

        VulkanPipeline *pipeline = nullptr;
    } _forwardPass;

    struct {
        VkDescriptorSet       set            = VK_NULL_HANDLE;
        VulkanPipeline       *pipeline       = nullptr;
        VulkanImage          *cubeMap        = nullptr;
        VulkanImageView      *cubeMapView    = nullptr;
        VkSampler             cubeMapSampler = nullptr;
        VkDescriptorImageInfo cubeMapDescInfo{};
    } _skyboxResource;

    struct {
        VulkanImage       *depthImage     = nullptr;
        VulkanImageView   *depthImageView = nullptr;
        VulkanRenderPass  *renderPass     = nullptr;
        VulkanFramebuffer *framebuffer    = nullptr;
        VkSampler          depthSampler   = VK_NULL_HANDLE;
    } _shaderPass;

    struct {
        VulkanBuffer                    *quadVB     = nullptr;
        VulkanRenderPass                *renderPass = nullptr;
        VulkanPipeline                  *pipeline   = nullptr;
        std::vector<VulkanImage *>       colorImages;
        std::vector<VulkanImageView *>   colorImageViews;
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VkSampler>           samplers;
        std::vector<VkDescriptorSet>     sets;
    } _postFxPass;

private:
    std::vector<std::shared_ptr<VulkanRenderData>> _renderList;
    std::deque<std::shared_ptr<VulkanUniformData>> _uniformList;

    std::vector<VkDescriptorBufferInfo> _cameraInfos{};
    std::vector<VkDescriptorBufferInfo> _lightInfos{};

private:
    VulkanDevice                   *m_pDevice   = nullptr;
    std::shared_ptr<VulkanRenderer> m_pRenderer = nullptr;
};
} // namespace vkl

#endif // VKSCENERENDERER_H_
