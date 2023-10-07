#ifndef VKSCENERENDERER_H_
#define VKSCENERENDERER_H_

#include "api/vulkan/device.h"
#include "renderer.h"
#include "scene/scene.h"

namespace aph::vk
{
class SceneRenderer : public Renderer
{
public:
    SceneRenderer(WSI* wsi, const RenderConfig& config);

    void load(Scene* scene);
    void cleanup();
    void update(float deltaTime);
    void recordAll();
    void recordShadow(CommandBuffer* pCommandBuffer);
    void recordDeferredGeometry(CommandBuffer* pCommandBuffer);
    void recordDeferredLighting(CommandBuffer* pCommandBuffer);
    void recordPostFX(CommandBuffer* pCommandBuffer);

private:
    void drawUI(float deltaTime);
    void _initShadow();
    void _initGbuffer();
    void _initGeneral();
    void _initSkybox();
    void _initGpuResources();
    void _initSet();
    void _loadScene();

private:
    enum SetLayoutIndex
    {
        SET_LAYOUT_SAMP,
        SET_LAYOUT_MAX,
    };

    enum SamplerIndex
    {
        SAMP_TEXTURE,
        SAMP_SHADOW,
        SAMP_CUBEMAP,
        SAMP_MAX,
    };

    enum PipelineIndex
    {
        PIPELINE_GRAPHICS_GEOMETRY,
        PIPELINE_GRAPHICS_LIGHTING,
        PIPELINE_GRAPHICS_SHADOW,
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
        BUFFER_INDIRECT_DRAW_CMD,
        BUFFER_INDIRECT_DISPATCH_CMD,
        BUFFER_MAX,
    };

    enum ImageIndex
    {
        IMAGE_GBUFFER_POSITION,
        IMAGE_GBUFFER_NORMAL,
        IMAGE_GBUFFER_ALBEDO,
        IMAGE_GBUFFER_EMISSIVE,
        IMAGE_GBUFFER_MRAO,
        IMAGE_GBUFFER_DEPTH,
        IMAGE_SHADOW_DEPTH,
        IMAGE_GENERAL_COLOR,
        IMAGE_GENERAL_COLOR_POSTFX,
        IMAGE_GENERAL_DEPTH,
        IMAGE_GENERAL_COLOR_MS,
        IMAGE_GENERAL_DEPTH_MS,
        IMAGE_SCENE_SKYBOX,
        IMAGE_SCENE_TEXTURES,
        IMAGE_MAX
    };

    enum ShaderProgramIdx
    {
        SHADER_PROGRAM_DEFERRED_GEOMETRY,
        SHADER_PROGRAM_DEFERRED_LIGHTING,
        SHADER_PROGRAM_SKYBOX,
        SHADER_PROGRAM_SHADOW,
        SHADER_PROGRAM_POSTFX,
        SHADER_PROGRAM_MAX
    };

    // TODO
    enum CommandBufferIdx
    {
        COMMAND_BUFFER_GEOMETRY = 0,
        COMMAND_BUFFER_SHADOW   = 1,
        COMMAND_BUFFER_LIGHTING = 2,
        COMMAND_BUFFER_POSTFX   = 3,
        COMMAND_BUFFER_MAX,
    };
    CommandBuffer* cb[2][COMMAND_BUFFER_MAX] = {};

    std::array<ShaderProgram*, SHADER_PROGRAM_MAX>   m_programs;
    std::array<Buffer*, BUFFER_MAX>                  m_buffers;
    std::array<Pipeline*, PIPELINE_MAX>              m_pipelines;
    std::array<DescriptorSetLayout*, SET_LAYOUT_MAX> m_setLayouts;
    std::array<Sampler*, SAMP_MAX>                   m_samplers;
    std::array<std::vector<Image*>, IMAGE_MAX>       m_images;

    // TODO remove
    std::vector<VkDescriptorSet> m_gbufferSets{};
    std::vector<VkDescriptorSet> m_postFxSets{};
    VkDescriptorSet              m_sceneSet{};
    VkDescriptorSet              m_samplerSet{};
    VkDescriptorSet              m_shadowSet{};
    VkDescriptorSet              m_geometrySet{};
    VkDescriptorSet              m_skyboxSet{};

private:
    Scene*                      m_scene = {};
    std::vector<SceneNode*>     m_meshNodeList;
    std::vector<Camera*>        m_cameraList;
    std::vector<Light*>         m_lightList;
    std::unique_ptr<ThreadPool> m_threadPool{};
};
}  // namespace aph::vk

#endif  // VKSCENERENDERER_H_
