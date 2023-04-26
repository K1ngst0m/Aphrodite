#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "api/vulkan/device.h"
#include "renderer.h"
#include "scene/scene.h"

namespace aph
{
class VulkanSceneRenderer : public VulkanRenderer
{
public:
    VulkanSceneRenderer(std::shared_ptr<Window> window, const RenderConfig& config);

    void load(Scene* scene);
    void cleanup();
    void update(float deltaTime);
    void recordAll();
    void recordDeferredGeometry(VulkanCommandBuffer* pCommandBuffer);
    void recordDeferredLighting(VulkanCommandBuffer* pCommandBuffer);
    void recordForward(VulkanCommandBuffer* pCommandBuffer);
    void recordPostFX(VulkanCommandBuffer* pCommandBuffer);

private:
    void drawUI(float deltaTime);
    void _initGbuffer();
    void _initGeneral();
    void _initSkybox();
    void _initPipeline();
    void _initSetLayout();
    void _initGpuResources();
    void _initSet();
    void _loadScene();

private:
    enum SetLayoutIndex
    {
        SET_LAYOUT_SAMP,
        // SET_LAYOUT_MATERIAL,
        SET_LAYOUT_SCENE,
        // SET_LAYOUT_OBJECT,
        SET_LAYOUT_POSTFX,
        SET_LAYOUT_GBUFFER,
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
        PIPELINE_GRAPHICS_GEOMETRY,
        PIPELINE_GRAPHICS_LIGHTING,
        PIPELINE_GRAPHICS_FORWARD,
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
        IMAGE_GBUFFER_EMISSIVE,
        IMAGE_GBUFFER_METALLIC_ROUGHNESS_AO,
        IMAGE_GBUFFER_DEPTH,
        IMAGE_GENERAL_COLOR,
        IMAGE_GENERAL_DEPTH,
        IMAGE_GENERAL_COLOR_MS,
        IMAGE_GENERAL_DEPTH_MS,
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
    Scene*                  m_scene = {};
    std::vector<SceneNode*> m_meshNodeList;
    std::vector<SceneNode*> m_cameraNodeList;
    std::vector<SceneNode*> m_lightNodeList;
};
}  // namespace aph

#endif  // VKSCENERENDERER_H_
