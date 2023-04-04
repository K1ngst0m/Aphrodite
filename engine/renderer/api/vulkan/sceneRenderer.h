#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "renderer/sceneRenderer.h"
#include "api/vulkan/device.h"
#include "scene/mesh.h"
#include "renderer.h"

namespace aph
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

struct MaterialGpuData
{
    VulkanBuffer *buffer = nullptr;
    VkDescriptorSet set = VK_NULL_HANDLE;
};

struct VulkanRenderData
{
    VulkanRenderData(std::shared_ptr<SceneNode> sceneNode)
        : m_node{std::move(sceneNode)}
    {}

    VulkanBuffer *m_vertexBuffer = nullptr;
    VulkanBuffer *m_indexBuffer = nullptr;

    VulkanBuffer *m_objectUB = nullptr;
    VkDescriptorSet m_objectSet = VK_NULL_HANDLE;

    std::shared_ptr<SceneNode> m_node = nullptr;
};

struct VulkanUniformData
{
    VulkanUniformData(std::shared_ptr<SceneNode> node)
        : m_node{std::move(node)}
    {}

    VulkanBuffer *m_buffer = nullptr;

    std::shared_ptr<SceneNode> m_node = nullptr;
    std::shared_ptr<UniformObject> m_object = nullptr;
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
    VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer);
    ~VulkanSceneRenderer() override = default;
    void loadResources() override;
    void cleanupResources() override;
    void update(float deltaTime) override;
    void recordDrawSceneCommands() override;

private:
    void _initSetLayout();
    void _initSampler();
    void _initRenderData();
    void _initSkyboxResource();
    void _initForward();
    void _initPostFx();
    void _loadScene();
    void _drawRenderData(const std::shared_ptr<VulkanRenderData> &renderData, VulkanPipeline *pipeline, VulkanCommandBuffer *drawCmd);

private:
    struct {
        VulkanDescriptorSetLayout * pSampler;
        VulkanDescriptorSetLayout * pMaterial;
        VulkanDescriptorSetLayout * pScene;
        VulkanDescriptorSetLayout * pObject;
        VulkanDescriptorSetLayout * pOffScreen;
    } m_setLayout;

    struct {
        VkSampler texture = VK_NULL_HANDLE;
        VkSampler shadow = VK_NULL_HANDLE;
        VkSampler postFX = VK_NULL_HANDLE;
        VkSampler cubeMap = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE;
    } m_sampler;

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

        std::vector<VulkanImage *> colorAttachments;
        std::vector<VulkanImage *> depthAttachments;
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
        std::vector<VulkanImage *> depthAttachments;
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
        VulkanPipeline *pipeline = nullptr;
        std::vector<VulkanImage *> colorAttachments;
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

    std::vector<std::shared_ptr<VulkanRenderData>> m_renderDataList;
    std::deque<std::shared_ptr<VulkanUniformData>> m_uniformDataList;

    std::vector<VkDescriptorBufferInfo> m_cameraInfos{};
    std::vector<VkDescriptorBufferInfo> m_lightInfos{};
    std::vector<VulkanImage*> m_textures{};

private:
    VulkanDevice *m_pDevice = nullptr;
    std::shared_ptr<VulkanRenderer> m_pRenderer = nullptr;
    std::unordered_map<std::shared_ptr<Material>, MaterialGpuData> m_materialDataMaps;
};
}  // namespace aph

#endif  // VKSCENERENDERER_H_
