#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "device.h"
#include "renderer/sceneRenderer.h"
#include "scene/mesh.h"
#include "vulkanRenderer.h"

namespace vkl
{
class ShaderPass;
class VulkanRenderer;
struct VulkanUniformData;
struct VulkanRenderData;

enum MaterialBindingBits
{
    MATERIAL_BINDING_NONE = 0,
    MATERIAL_BINDING_BASECOLOR = (1 << 0),
    MATERIAL_BINDING_NORMAL = (1 << 1),
    MATERIAL_BINDING_PHYSICAL = (1 << 2),
    MATERIAL_BINDING_AO = (1 << 3),
    MATERIAL_BINDING_EMISSIVE = (1 << 4),
    MATERIAL_BINDING_UNLIT = MATERIAL_BINDING_BASECOLOR,
    MATERIAL_BINDING_DEFAULTLIT = (MATERIAL_BINDING_BASECOLOR | MATERIAL_BINDING_NORMAL),
    MATERIAL_BINDING_PBR = (MATERIAL_BINDING_BASECOLOR | MATERIAL_BINDING_NORMAL |
                            MATERIAL_BINDING_PHYSICAL | MATERIAL_BINDING_AO | MATERIAL_BINDING_EMISSIVE),
};

struct GpuTexture
{
    VulkanImage *image = nullptr;
    VulkanImageView *imageView = nullptr;
    VkDescriptorImageInfo descriptorInfo;
};

struct MaterialGpuData
{
    VulkanBuffer *buffer = nullptr;
    VkDescriptorSet set = VK_NULL_HANDLE;
};

struct SceneInfo
{
    glm::vec4 ambient{ 0.04f };
    uint32_t cameraCount = 0;
    uint32_t lightCount = 0;
};

struct ObjectInfo
{
    glm::mat4 matrix = glm::mat4(1.0f);
};

class VulkanSceneRenderer : public SceneRenderer
{
public:
    static std::unique_ptr<VulkanSceneRenderer> Create(const std::shared_ptr<VulkanRenderer> &renderer);
    VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update(float deltaTime) override;
    void drawScene() override;

private:
    void _initSampler();
    void _initRenderData();
    void _initSkyboxResource();
    void _initForward();
    void _initPostFx();
    void _initShadow();
    void _loadScene();
    void _drawRenderData(const std::shared_ptr<VulkanRenderData> &renderData, VulkanPipeline *pipeline, VulkanCommandBuffer *drawCmd);

private:
    struct {
        VkSampler texture = VK_NULL_HANDLE;
        VkSampler shadow = VK_NULL_HANDLE;
        VkSampler postFX = VK_NULL_HANDLE;
        VkSampler cubeMap = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE;
    } m_samplers;

    struct PASS_FORWARD
    {
        enum SetBinding
        {
            SET_SCENE = 0,
            SET_OBJECT = 1,
            SET_MATERIAL = 2,
            SET_SAMPLER = 3,
            SET_SKYBOX = 4,
        };

        VulkanPipeline *pipeline = nullptr;
        vkl::VulkanRenderPass *renderPass = nullptr;

        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VulkanImage *> colorImages;
        std::vector<VulkanImageView *> colorImageViews;
        std::vector<VulkanImage *> depthImages;
        std::vector<VulkanImageView *> depthImageViews;
    } m_forwardPass;

    struct PASS_SHADOW
    {
        enum SetBinding
        {
            SET_SCENE = 0,
        };

        const uint32_t dim = 2048;
        const VkFilter filter = VK_FILTER_LINEAR;
        VulkanPipeline *pipeline = nullptr;
        VulkanRenderPass *renderPass = nullptr;
        std::vector<VulkanImage *> depthImages;
        std::vector<VulkanImageView *> depthImageViews;
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VkDescriptorSet> cameraSets;
    } m_shadowPass;

    struct PASS_POSTFX
    {
        enum SetBinding
        {
            SET_OFFSCREEN = 0,
            SET_SAMPLER = 1,
        };

        VulkanBuffer *quadVB = nullptr;
        VulkanRenderPass *renderPass = nullptr;
        VulkanPipeline *pipeline = nullptr;
        std::vector<VulkanImage *> colorImages;
        std::vector<VulkanImageView *> colorImageViews;
        std::vector<VulkanFramebuffer *> framebuffers;
        std::vector<VkDescriptorSet> sets;
    } m_postFxPass;

private:
    struct
    {
        VkDescriptorSet set = VK_NULL_HANDLE;
        VulkanPipeline *pipeline = nullptr;
        VulkanImage *cubeMap = nullptr;
        VulkanImageView *cubeMapView = nullptr;
        VkDescriptorImageInfo cubeMapDescInfo{};
    } m_skyboxResource;

    std::vector<VkDescriptorSet> m_sceneSets;
    SceneInfo m_sceneInfo{};
    VulkanBuffer *m_sceneInfoUB = nullptr;

    std::vector<std::shared_ptr<VulkanRenderData>> m_renderList;
    std::deque<std::shared_ptr<VulkanUniformData>> m_uniformList;

    std::vector<VkDescriptorBufferInfo> m_cameraInfos{};
    std::vector<VkDescriptorBufferInfo> m_lightInfos{};
    std::vector<GpuTexture> m_textures{};

private:
    VulkanDevice *m_pDevice = nullptr;
    std::shared_ptr<VulkanRenderer> m_pRenderer = nullptr;
    std::unordered_map<std::shared_ptr<Material>, MaterialGpuData> m_materialDataMaps;
};
}  // namespace vkl

#endif  // VKSCENERENDERER_H_
