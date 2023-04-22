#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "api/vulkan/device.h"
#include "renderer.h"
#include "uiRenderer.h"
#include "renderer/sceneRenderer.h"

namespace aph
{
class VulkanSceneRenderer final : public ISceneRenderer, public VulkanRenderer
{
public:
    VulkanSceneRenderer(std::shared_ptr<Window> window, const RenderConfig& config);

    void loadResources() override;
    void cleanupResources() override;
    void update(float deltaTime) override;
    void recordDrawSceneCommands() override;
    void recordDrawSceneCommands(VulkanCommandBuffer* pCommandBuffer);
    void recordPostFxCommands(VulkanCommandBuffer* pCommandBuffer);
    void setUIRenderer(const std::unique_ptr<VulkanUIRenderer>& renderer) { m_pUIRenderer = renderer.get(); }

private:
    void _updateUI(float deltaTime);
    void _initSetLayout();
    void _initSet();
    void _initForward();
    void _initSkybox();
    void _initPipeline();
    void _loadScene();
    void _initGpuResources();

private:
    enum SetLayoutIndex
    {
        SET_LAYOUT_SAMP,
        // SET_LAYOUT_MATERIAL,
        SET_LAYOUT_SCENE,
        // SET_LAYOUT_OBJECT,
        SET_LAYOUT_POSTFX,
        SET_LAYOUT_MAX,
    };

    enum SamplerIndex
    {
        SAMP_TEXTURE,
        // SAMP_SHADOW,
        // SAMP_POSTFX,
        SAMP_CUBEMAP,
        SAMP_MAX,
    };

    enum PipelineIndex
    {
        PIPELINE_GRAPHICS_FORWARD,
        // PIPELINE_GRAPHICS_SHADOW,
        PIPELINE_GRAPHICS_SKYBOX,
        PIPELINE_COMPUTE_POSTFX,
        PIPELINE_MAX,
    };

    enum BufferIndex
    {
        BUFFER_CUBE_VERTEX,
        BUFFER_SCENE_VERTEX,
        BUFFER_SCENE_INDEX,
        BUFFER_SCENE_INFO,
        BUFFER_SCENE_MATERIAL,
        BUFFER_SCENE_LIGHT,
        BUFFER_SCENE_CAMERA,
        BUFFER_SCENE_TRANSFORM,
        BUFFER_MAX,
    };

    enum ImageIndex
    {
        IMAGE_GBUFFER_POSITION,
        IMAGE_GBUFFER_NORMAL,
        IMAGE_GBUFFER_ALBEDO,
        IMAGE_FORWARD_COLOR,
        IMAGE_FORWARD_DEPTH,
        IMAGE_FORWARD_COLOR_MS,
        IMAGE_FORWARD_DEPTH_MS,
        IMAGE_SCENE_SKYBOX,
        IMAGE_SCENE_TEXTURES,
        IMAGE_MAX
    };

    std::array<VulkanBuffer*, BUFFER_MAX>                  m_buffers;
    std::array<VulkanPipeline*, PIPELINE_MAX>              m_pipelines;
    std::array<VulkanDescriptorSetLayout*, SET_LAYOUT_MAX> m_setLayouts;
    std::array<VkSampler, SAMP_MAX>                        m_samplers;
    std::array<std::vector<VulkanImage*>, IMAGE_MAX>       m_images;
    VkDescriptorSet                                        m_sceneSet{};
    VkDescriptorSet                                        m_samplerSet{};

    VulkanImageView* m_pCubeMapView{};

private:
    std::vector<std::shared_ptr<SceneNode>> m_meshNodeList;
    std::vector<std::shared_ptr<SceneNode>> m_cameraNodeList;
    std::vector<std::shared_ptr<SceneNode>> m_lightNodeList;

private:
    VulkanUIRenderer* m_pUIRenderer = {};
};
}  // namespace aph

#endif  // VKSCENERENDERER_H_
