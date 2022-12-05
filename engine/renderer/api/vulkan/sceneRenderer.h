#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "device.h"
#include "renderer/sceneRenderer.h"
#include "vulkanRenderer.h"

namespace vkl {
class ShaderPass;
class VulkanRenderer;
struct VulkanUniformData;
struct VulkanRenderData;

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

struct SceneInfo {
    glm::vec4 ambient{0.04f};
    uint32_t  cameraCount = 0;
    uint32_t  lightCount  = 0;
};

class VulkanSceneRenderer : public SceneRenderer {
public:
    static std::unique_ptr<VulkanSceneRenderer> Create(const std::shared_ptr<VulkanRenderer> &renderer);
    VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update(float deltaTime) override;
    void drawScene() override;

private:
    void _initRenderData();
    void _initCommonResource();
    void _initSkyboxResource();
    void _initForwardResource();
    void _initPostFxResource();
    void _initShadowPassResource();
    void _loadSceneNodes();

private:
    VulkanDescriptorSetLayout *pSceneLayout       = nullptr;
    VulkanDescriptorSetLayout *pPBRMaterialLayout = nullptr;

    struct PASS_FORWARD {
        enum SetBinding {
            SET_SCENE    = 0,
            SET_OBJECT   = 1,
            SET_MATERIAL = 2,
            SET_SKYBOX   = 3,
        };

        VulkanPipeline        *pipeline       = nullptr;
        vkl::VulkanRenderPass *renderPass     = nullptr;

        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VulkanImage *>       colorImages;
        std::vector<VulkanImageView *>   colorImageViews;
        std::vector<VulkanImage *>       depthImages;
        std::vector<VulkanImageView *>   depthImageViews;
    } _forwardPass;

    struct PASS_SHADOW {
        enum SetBinding {
            SET_SCENE = 0,
        };

        const uint32_t                   dim        = 2048;
        const VkFilter                   filter     = VK_FILTER_LINEAR;
        VulkanPipeline                  *pipeline   = nullptr;
        VulkanRenderPass                *renderPass = nullptr;
        VkSampler                        sampler    = VK_NULL_HANDLE;
        std::vector<VulkanImage *>       depthImages;
        std::vector<VulkanImageView *>   depthImageViews;
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VkDescriptorSet>     cameraSets;
    } _shadowPass;

    struct PASS_POSTFX {
        enum SetBinding {
            SET_OFFSCREEN = 0,
        };

        VulkanBuffer                    *quadVB     = nullptr;
        VulkanRenderPass                *renderPass = nullptr;
        VulkanPipeline                  *pipeline   = nullptr;
        VkSampler                        sampler    = VK_NULL_HANDLE;
        std::vector<VulkanImage *>       colorImages;
        std::vector<VulkanImageView *>   colorImageViews;
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VkDescriptorSet>     sets;
    } _postFxPass;

private:
    struct {
        VkDescriptorSet       set            = VK_NULL_HANDLE;
        VulkanPipeline       *pipeline       = nullptr;
        VulkanImage          *cubeMap        = nullptr;
        VulkanImageView      *cubeMapView    = nullptr;
        VkSampler             cubeMapSampler = nullptr;
        VkDescriptorImageInfo cubeMapDescInfo{};
    } _skyboxResource;

    std::vector<VkDescriptorSet> _sceneSets;
    SceneInfo                    _sceneInfo{};
    VulkanBuffer                *_sceneInfoUB = nullptr;

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
