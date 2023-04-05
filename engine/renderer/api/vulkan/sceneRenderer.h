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

struct VulkanRenderData
{
    VulkanRenderData(std::shared_ptr<SceneNode> sceneNode)
        : m_node{std::move(sceneNode)}
    {}

    VulkanBuffer *m_vertexBuffer {};
    VulkanBuffer *m_indexBuffer {};

    VulkanBuffer *m_objectUB {};
    VkDescriptorSet m_objectSet {};

    std::shared_ptr<SceneNode> m_node {};
};

struct VulkanUniformData
{
    VulkanUniformData(std::shared_ptr<SceneNode> node)
        : m_node{std::move(node)}
    {}

    void update()
    {
        m_buffer->copyTo(m_object->getData());
    }

    VulkanBuffer *m_buffer {};

    std::shared_ptr<SceneNode> m_node {};
    std::shared_ptr<UniformObject> m_object {};
};

struct SceneInfo
{
    glm::vec4 ambient{ 0.04f };
    uint32_t cameraCount {};
    uint32_t lightCount {};
};

struct ObjectInfo
{
    glm::mat4 matrix {1.0f};
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
    void _initForward();
    void _initPostFx();
    void _loadScene();
    void _drawRenderData(const std::shared_ptr<VulkanRenderData> &renderData, VulkanPipeline *pipeline, VulkanCommandBuffer *drawCmd);

private:
    enum SetLayoutIndex{
        SET_LAYOUT_SAMP = 0,
        SET_LAYOUT_MATERIAL = 1,
        SET_LAYOUT_SCENE = 2,
        SET_LAYOUT_OBJECT = 3,
        SET_LAYOUT_OFFSCR = 4,
        SET_LAYOUT_MAX = 5,
    };

    enum SamplerIndex{
        SAMP_TEXTURE = 0,
        SAMP_SHADOW = 1,
        SAMP_POSTFX = 2,
        SAMP_CUBEMAP = 3,
        SAMP_MAX = 4,
    };

    std::array<VulkanDescriptorSetLayout*, SET_LAYOUT_MAX> m_setLayouts;
    std::array<VkSampler, SAMP_MAX> m_samplers;

    VkDescriptorSet m_sceneSet   {};
    VkDescriptorSet m_samplerSet {};

    struct PASS_FORWARD
    {
        VulkanPipeline *pipeline {};
        std::vector<VulkanImage *> colorAttachments;
        std::vector<VulkanImage *> depthAttachments;
    } m_forwardPass;

    struct PASS_SHADOW
    {
        const uint32_t dim {2048};
        const VkFilter filter {VK_FILTER_LINEAR};
        VulkanPipeline *pipeline {};
        std::vector<VulkanImage *> depthAttachments;
        std::vector<VkDescriptorSet> cameraSets;
    } m_shadowPass;

    struct PASS_POSTFX
    {
        VulkanBuffer *quadVB {};
        VulkanPipeline *pipeline {};
        std::vector<VulkanImage *> colorAttachments {};
        std::vector<VkDescriptorSet> sets {};
    } m_postFxPass;

private:
    SceneInfo m_sceneInfo{};

    std::vector<std::shared_ptr<VulkanRenderData>> m_renderDataList;
    std::deque<std::shared_ptr<VulkanUniformData>> m_uniformDataList;

    std::vector<VkDescriptorBufferInfo> m_cameraInfos{};
    std::vector<VkDescriptorBufferInfo> m_lightInfos{};
    std::vector<VulkanImage*> m_textures{};

private:
    VulkanDevice *m_pDevice {};
    std::shared_ptr<VulkanRenderer> m_pRenderer {};
    std::unordered_map<std::shared_ptr<Material>, VkDescriptorSet> m_materialSetMaps {};
};
}  // namespace aph

#endif  // VKSCENERENDERER_H_
