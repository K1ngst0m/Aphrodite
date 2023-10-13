#include "sceneRenderer.h"

#include "ui/ui.h"

#include "api/gpuResource.h"
#include "api/vulkan/vkUtils.h"
#include "common/assetManager.h"

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"

#include "api/vulkan/device.h"

#include <glm/detail/type_mat.hpp>
#include <glm/gtx/string_cast.hpp>
#include <type_traits>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_glfw.h>

namespace aph::vk
{
struct SceneInfo
{
    glm::vec4 ambient{0.04f};
    uint32_t  cameraCount{};
    uint32_t  lightCount{};
};

struct CameraInfo
{
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::vec4 viewPos{1.0f};
};

struct LightInfo
{
    glm::vec4 color{1.0f};
    glm::vec4 position{1.0f};
    glm::vec4 direction{1.0f};
    uint32_t  lightType{};
};

struct ObjectInfo
{
    uint32_t nodeId{};
    uint32_t materialId{};
};
}  // namespace aph::vk

namespace aph::vk
{
SceneRenderer::SceneRenderer(WSI* wsi, const RenderConfig& config) : Renderer(wsi, config)
{
    m_threadPool = std::make_unique<ThreadPool>(10);
}

void SceneRenderer::load(Scene* scene)
{
    m_scene = scene;

    _loadScene();
    _initGpuResources();

    _initShadow();
    _initGbuffer();
    _initGeneral();
    _initSkybox();

    _initSet();

    auto* queue = getDefaultQueue(QueueType::GRAPHICS);
    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        m_pDevice->allocateThreadCommandBuffers(COMMAND_BUFFER_MAX, cb[idx], queue);
    }
}

void SceneRenderer::cleanup()
{
    for(auto* pipeline : m_pipelines)
    {
        m_pDevice->destroy(pipeline);
    }

    for(const auto& images : m_images)
    {
        for(auto* image : images)
        {
            m_pDevice->destroy(image);
        }
    }

    for(auto* buffer : m_buffers)
    {
        m_pDevice->destroy(buffer);
    }

    for(const auto& sampler : m_samplers)
    {
        m_pDevice->destroy(sampler);
    }
}

void SceneRenderer::recordAll()
{
    auto* queue     = getDefaultQueue(QueueType::GRAPHICS);
    auto* currentCB = cb[m_frameIdx];

    {
        m_threadPool->AddTask([=]() {
            currentCB[COMMAND_BUFFER_SHADOW]->reset();
            currentCB[COMMAND_BUFFER_SHADOW]->begin();
            recordShadow(currentCB[COMMAND_BUFFER_SHADOW]);
            currentCB[COMMAND_BUFFER_SHADOW]->end();
        });

        m_threadPool->AddTask([=]() {
            currentCB[COMMAND_BUFFER_GEOMETRY]->reset();
            currentCB[COMMAND_BUFFER_GEOMETRY]->begin();
            recordDeferredGeometry(currentCB[COMMAND_BUFFER_GEOMETRY]);
            currentCB[COMMAND_BUFFER_GEOMETRY]->end();
        });

        m_threadPool->AddTask([=]() {
            currentCB[COMMAND_BUFFER_LIGHTING]->reset();
            currentCB[COMMAND_BUFFER_LIGHTING]->begin();
            recordDeferredLighting(currentCB[COMMAND_BUFFER_LIGHTING]);
            currentCB[COMMAND_BUFFER_LIGHTING]->end();
        });

        m_threadPool->AddTask([=]() {
            currentCB[COMMAND_BUFFER_POSTFX]->reset();
            currentCB[COMMAND_BUFFER_POSTFX]->begin();
            recordPostFX(currentCB[COMMAND_BUFFER_POSTFX]);
            currentCB[COMMAND_BUFFER_POSTFX]->end();
        });
    }

    std::vector<QueueSubmitInfo2> submitInfos(COMMAND_BUFFER_MAX);
    {
        // TODO timeline
        VkSemaphore timelineMain = VK_NULL_HANDLE;
        // VkSemaphore timelineMain = acquireTimelineMain();
        VkSemaphore timelineShadow{};

        m_pSyncPrimitivesPool->acquireTimelineSemaphore(1, &timelineShadow);

        // 1. geometry && shadow
        submitInfos[COMMAND_BUFFER_GEOMETRY].signals.push_back({.semaphore = timelineMain, .value = 1});
        submitInfos[COMMAND_BUFFER_SHADOW].signals.push_back({.semaphore = timelineShadow, .value = UINT64_MAX});

        // 2. lighting
        submitInfos[COMMAND_BUFFER_LIGHTING].waits.push_back({.semaphore = timelineShadow, .value = UINT64_MAX});
        submitInfos[COMMAND_BUFFER_LIGHTING].waits.push_back({.semaphore = timelineMain, .value = 1});

        submitInfos[COMMAND_BUFFER_LIGHTING].signals.push_back({.semaphore = timelineMain, .value = 2});

        // 3. postfx
        submitInfos[COMMAND_BUFFER_POSTFX].waits.push_back({.semaphore = m_renderSemaphore[m_frameIdx]});
        submitInfos[COMMAND_BUFFER_POSTFX].waits.push_back({.semaphore = timelineMain, .value = 2});

        submitInfos[COMMAND_BUFFER_POSTFX].signals.push_back({.semaphore = m_presentSemaphore[m_frameIdx]});
        submitInfos[COMMAND_BUFFER_POSTFX].signals.push_back({.semaphore = timelineMain, .value = UINT64_MAX});
    }

    for(auto idx = 0; idx < COMMAND_BUFFER_MAX; idx++)
    {
        submitInfos[idx].commands.push_back({.commandBuffer = cb[m_frameIdx][idx]->getHandle()});
    }

    m_threadPool->Wait();
    VK_CHECK_RESULT(queue->submit(submitInfos));
}

void SceneRenderer::update(float deltaTime)
{
    m_scene->update(deltaTime);

    {
        SceneInfo sceneInfo = {
            .ambient     = glm::vec4(m_scene->getAmbient(), 0.0f),
            .cameraCount = static_cast<uint32_t>(m_cameraList.size()),
            .lightCount  = static_cast<uint32_t>(m_lightList.size()),
        };
        m_buffers[BUFFER_SCENE_INFO]->write(&sceneInfo, 0, sizeof(SceneInfo));
    }

    for(uint32_t idx = 0; idx < m_meshNodeList.size(); idx++)
    {
        const auto& node = m_meshNodeList[idx];
        auto        data = node->getTransform();
        m_buffers[BUFFER_SCENE_TRANSFORM]->write(&data, sizeof(glm::mat4) * idx, sizeof(glm::mat4));
    }

    for(uint32_t idx = 0; idx < m_cameraList.size(); idx++)
    {
        const auto& camera = m_cameraList[idx];
        CameraInfo  cameraData{
             .view    = camera->m_view,
             .proj    = camera->m_projection,
             .viewPos = camera->m_view[3],
        };
        m_buffers[BUFFER_SCENE_CAMERA]->write(&cameraData, sizeof(CameraInfo) * idx, sizeof(CameraInfo));
    }

    for(uint32_t idx = 0; idx < m_lightList.size(); idx++)
    {
        const auto& light = m_lightList[idx];
        LightInfo   lightData{
              .color     = {light->m_color * light->m_intensity, 1.0f},
              .position  = {light->m_position, 1.0f},
              .direction = {light->m_direction, 1.0f},
              .lightType = aph::utils::getUnderLyingType(light->m_type),
        };
        m_buffers[BUFFER_SCENE_LIGHT]->write(&lightData, sizeof(LightInfo) * idx, sizeof(LightInfo));
    }

    drawUI(deltaTime);
    updateUIDrawData(deltaTime);
}

void SceneRenderer::_initSet()
{
    VK_LOG_DEBUG("Init descriptor set.");
    VkDescriptorBufferInfo sceneBufferInfo{m_buffers[BUFFER_SCENE_INFO]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo cameraBufferInfo{m_buffers[BUFFER_SCENE_CAMERA]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo lightBufferInfo{m_buffers[BUFFER_SCENE_LIGHT]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo materialBufferInfo{m_buffers[BUFFER_SCENE_MATERIAL]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo transformBufferInfo{m_buffers[BUFFER_SCENE_TRANSFORM]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorImageInfo  skyBoxImageInfo{nullptr, m_images[IMAGE_SCENE_SKYBOX][0]->getView()->getHandle(),
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    std::vector<VkDescriptorImageInfo> textureInfos{};
    for(auto& texture : m_images[IMAGE_SCENE_TEXTURES])
    {
        textureInfos.push_back({
            .imageView   = texture->getView()->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
    }
    // scene
    {
        m_sceneSet = m_pipelines[PIPELINE_GRAPHICS_LIGHTING]->getProgram()->getSetLayout(0)->allocateSet();
        std::vector<VkWriteDescriptorSet> writes{
            init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &sceneBufferInfo),
            init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &cameraBufferInfo),
            init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &lightBufferInfo),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    // geometry
    {
        m_geometrySet = m_pipelines[PIPELINE_GRAPHICS_GEOMETRY]->getProgram()->getSetLayout(0)->allocateSet();
        std::vector<VkWriteDescriptorSet> writes{
            init::writeDescriptorSet(m_geometrySet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &transformBufferInfo),
            init::writeDescriptorSet(m_geometrySet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &cameraBufferInfo),
            init::writeDescriptorSet(m_geometrySet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4, textureInfos.data(),
                                     textureInfos.size()),
            init::writeDescriptorSet(m_geometrySet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, &materialBufferInfo),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    // skybox
    {
        m_skyboxSet = m_pipelines[PIPELINE_GRAPHICS_SKYBOX]->getProgram()->getSetLayout(0)->allocateSet();
        std::vector<VkWriteDescriptorSet> writes{
            init::writeDescriptorSet(m_skyboxSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &cameraBufferInfo),
            init::writeDescriptorSet(m_skyboxSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 6, &skyBoxImageInfo),
        };

        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    // postfx
    {
        m_postFxSets.resize(m_config.maxFrames);
        for(uint32_t idx = 0; idx < m_config.maxFrames; idx++)
        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView   = m_images[IMAGE_GENERAL_COLOR][idx]->getView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo outputImageInfo{.imageView   = m_pSwapChain->getImage()->getView()->getHandle(),
                                                  .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

            m_postFxSets[idx] = m_pipelines[PIPELINE_COMPUTE_POSTFX]->getProgram()->getSetLayout(0)->allocateSet();
            std::vector<VkWriteDescriptorSet> writes{
                init::writeDescriptorSet(m_postFxSets[idx], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &inputImageInfo),
                init::writeDescriptorSet(m_postFxSets[idx], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &outputImageInfo),
            };
            vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
        }
    }

    // gbuffer
    {
        m_gbufferSets.resize(m_config.maxFrames);
        for(uint32_t idx = 0; idx < m_config.maxFrames; idx++)
        {
            ImageView* positionAttachment = m_images[IMAGE_GBUFFER_POSITION][idx]->getView();
            ImageView* normalAttachment   = m_images[IMAGE_GBUFFER_NORMAL][idx]->getView();
            ImageView* albedoAttachment   = m_images[IMAGE_GBUFFER_ALBEDO][idx]->getView();
            ImageView* mraoAttachment     = m_images[IMAGE_GBUFFER_MRAO][idx]->getView();
            ImageView* emissiveAttachment = m_images[IMAGE_GBUFFER_EMISSIVE][idx]->getView();
            ImageView* pShadowMap         = m_images[IMAGE_SHADOW_DEPTH][idx]->getView();

            VkDescriptorImageInfo posImageInfo{.imageView   = positionAttachment->getHandle(),
                                               .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo normalImageInfo{.imageView   = normalAttachment->getHandle(),
                                                  .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo albedoImageInfo{.imageView   = albedoAttachment->getHandle(),
                                                  .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo mraoImageInfo{.imageView   = mraoAttachment->getHandle(),
                                                .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo emissiveImageInfo{.imageView   = emissiveAttachment->getHandle(),
                                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo shadowMapInfo{.imageView   = pShadowMap->getHandle(),
                                                .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            m_gbufferSets[idx] = m_pipelines[PIPELINE_GRAPHICS_LIGHTING]->getProgram()->getSetLayout(2)->allocateSet();
            std::vector<VkWriteDescriptorSet> writes{
                init::writeDescriptorSet(m_gbufferSets[idx], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, &posImageInfo),
                init::writeDescriptorSet(m_gbufferSets[idx], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, &normalImageInfo),
                init::writeDescriptorSet(m_gbufferSets[idx], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2, &albedoImageInfo),
                init::writeDescriptorSet(m_gbufferSets[idx], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3, &mraoImageInfo),
                init::writeDescriptorSet(m_gbufferSets[idx], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4, &emissiveImageInfo),
                init::writeDescriptorSet(m_gbufferSets[idx], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5, &shadowMapInfo),
            };

            vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
        }
    }

    // shadow
    {
        m_shadowSet = m_pipelines[PIPELINE_GRAPHICS_SHADOW]->getProgram()->getSetLayout(0)->allocateSet();
        std::vector<VkWriteDescriptorSet> writes{
            init::writeDescriptorSet(m_shadowSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &transformBufferInfo),
            init::writeDescriptorSet(m_shadowSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &cameraBufferInfo),
        };

        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    // sampler
    {
        m_samplerSet = m_pipelines[PIPELINE_GRAPHICS_LIGHTING]->getProgram()->getSetLayout(1)->allocateSet();
        VkDescriptorImageInfo sampTextureInfo{.sampler = m_samplers[SAMP_TEXTURE]->getHandle()};
        VkDescriptorImageInfo sampShadowInfo{.sampler = m_samplers[SAMP_SHADOW]->getHandle()};
        VkDescriptorImageInfo sampCubemapInfo{.sampler = m_samplers[SAMP_CUBEMAP]->getHandle()};

        std::vector<VkWriteDescriptorSet> writes{
            init::writeDescriptorSet(m_samplerSet, VK_DESCRIPTOR_TYPE_SAMPLER, 0, &sampTextureInfo),
            init::writeDescriptorSet(m_samplerSet, VK_DESCRIPTOR_TYPE_SAMPLER, 1, &sampShadowInfo),
            init::writeDescriptorSet(m_samplerSet, VK_DESCRIPTOR_TYPE_SAMPLER, 2, &sampCubemapInfo),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }
}

void SceneRenderer::_loadScene()
{
    m_scene->getRootNode()->traversalChildren([&](SceneNode* node) {
        switch(node->getAttachType())
        {
        case ObjectType::MESH:
        {
            m_meshNodeList.push_back(node);
        }
        break;
        case ObjectType::CAMERA:
        {
            m_cameraList.push_back(node->getObject<Camera>());
        }
        break;
        case ObjectType::LIGHT:
        {
            m_lightList.push_back(node->getObject<Light>());
        }
        break;
        default:
            break;
        }
    });

    {
        // TODO for testing
        auto* cam         = new Camera{CameraType::PERSPECTIVE};
        cam->m_view       = glm::lookAt(glm::vec3(1.0f), glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        cam->m_projection = glm::perspective(glm::radians(100.0f), 1.0f, 0.1f, 64.0f);
        m_cameraList.push_back(cam);
    }
}

void SceneRenderer::_initGbuffer()
{
    VK_LOG_DEBUG("Init deferred pass.");
    VkExtent2D imageExtent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight()};
    m_images[IMAGE_GBUFFER_ALBEDO].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_NORMAL].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_POSITION].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_EMISSIVE].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_MRAO].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_DEPTH].resize(m_config.maxFrames);

    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        {
            auto& position = m_images[IMAGE_GBUFFER_POSITION][idx];
            auto& normal   = m_images[IMAGE_GBUFFER_NORMAL][idx];
            auto& albedo   = m_images[IMAGE_GBUFFER_ALBEDO][idx];
            auto& emissive = m_images[IMAGE_GBUFFER_EMISSIVE][idx];
            auto& mrao     = m_images[IMAGE_GBUFFER_MRAO][idx];

            ImageCreateInfo createInfo{
                .extent    = {imageExtent.width, imageExtent.height, 1},
                .usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain    = ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = VK_FORMAT_R16G16B16A16_SFLOAT,
            };
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &position));
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &normal));
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &mrao));

            createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &albedo));
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &emissive));
        }

        {
            auto& depth = m_images[IMAGE_GBUFFER_DEPTH][idx];

            ImageCreateInfo createInfo{
                .extent = {imageExtent.width, imageExtent.height, 1},
                .usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .domain = ImageDomain::Device,
                .format = m_pDevice->getDepthFormat(),
                .tiling = VK_IMAGE_TILING_OPTIMAL,
            };
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &depth));
            m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](auto* cmd) {
                cmd->transitionImageLayout(depth, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
            });
        }
    }

    // geometry graphics pipeline
    {
        // TODO vertex input
        auto                       shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        std::vector<VkFormat>      colorFormats = {};
        GraphicsPipelineCreateInfo createInfo{
            .pVertex   = getShaders(shaderDir / "geometry.vert"),
            .pFragment = getShaders(shaderDir / "geometry.frag"),
            .color =
                {
                    {.format = VK_FORMAT_R16G16B16A16_SFLOAT},
                    {.format = VK_FORMAT_R16G16B16A16_SFLOAT},
                    {.format = VK_FORMAT_R8G8B8A8_UNORM},
                    {.format = VK_FORMAT_R16G16B16A16_SFLOAT},
                    {.format = VK_FORMAT_R8G8B8A8_UNORM},
                },
        };

        VK_CHECK_RESULT(m_pDevice->create(createInfo, &m_pipelines[PIPELINE_GRAPHICS_GEOMETRY]));
    }

    // deferred light pbr pipeline
    {
        // TODO vertex input
        auto                       shaderDir = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        GraphicsPipelineCreateInfo createInfo{
            .pVertex   = getShaders(shaderDir / "pbr_deferred.vert"),
            .pFragment = getShaders(shaderDir / "pbr_deferred.frag"),
            .color     = {{.format = m_pSwapChain->getFormat()}},
        };

        VK_CHECK_RESULT(m_pDevice->create(createInfo, &m_pipelines[PIPELINE_GRAPHICS_LIGHTING]));
    }
}

void SceneRenderer::_initGeneral()
{
    VK_LOG_DEBUG("Init general pass.");

    m_images[IMAGE_GENERAL_COLOR].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_COLOR_POSTFX].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_DEPTH].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_COLOR_MS].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_DEPTH_MS].resize(m_config.maxFrames);

    // frame buffer
    VkExtent2D imageExtent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight()};
    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        {
            auto& colorImage   = m_images[IMAGE_GENERAL_COLOR][idx];
            auto& colorImageMS = m_images[IMAGE_GENERAL_COLOR_MS][idx];
            auto& colorPostFx  = m_images[IMAGE_GENERAL_COLOR_POSTFX][idx];

            ImageCreateInfo createInfo{
                .extent = {imageExtent.width, imageExtent.height, 1},
                .usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain = ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = VK_FORMAT_B8G8R8A8_UNORM,
            };
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &colorImage));
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &colorPostFx));
            createInfo.samples = m_sampleCount;
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &colorImageMS));
        }

        {
            auto&           depthImage   = m_images[IMAGE_GENERAL_DEPTH][idx];
            auto&           depthImageMS = m_images[IMAGE_GENERAL_DEPTH_MS][idx];
            ImageCreateInfo createInfo{
                .extent = {imageExtent.width, imageExtent.height, 1},
                .usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .domain = ImageDomain::Device,
                .format = m_pDevice->getDepthFormat(),
                .tiling = VK_IMAGE_TILING_OPTIMAL,
            };
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &depthImage));
            createInfo.samples = m_sampleCount;
            VK_CHECK_RESULT(m_pDevice->create(createInfo, &depthImageMS));

            m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](auto* cmd) {
                cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
                cmd->transitionImageLayout(depthImageMS, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
            });
        }
    }

    // postfx compute pipeline
    {
        std::filesystem::path shaderDir = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        {
            VK_CHECK_RESULT(m_pDevice->create({.pCompute = getShaders(shaderDir / "postFX.comp")},
                                              &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
        }
    }
}

void SceneRenderer::_initGpuResources()
{
    VK_LOG_DEBUG("Init GPU resources.");
    // create scene info buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(sizeof(SceneInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->create(createInfo, &m_buffers[BUFFER_SCENE_INFO]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_INFO]);
    }
    // create camera buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(m_cameraList.size() * sizeof(CameraInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->create(createInfo, &m_buffers[BUFFER_SCENE_CAMERA]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_CAMERA]);
    }

    // create light buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(m_lightList.size() * sizeof(LightInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->create(createInfo, &m_buffers[BUFFER_SCENE_LIGHT]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_LIGHT]);
    }

    // create transform buffer
    {
        BufferCreateInfo createInfo{.size   = static_cast<uint32_t>(m_meshNodeList.size() * sizeof(glm::mat4)),
                                    .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    .domain = BufferDomain::Host};
        m_pDevice->create(createInfo, &m_buffers[BUFFER_SCENE_TRANSFORM]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_TRANSFORM]);
    }

    // create index buffer
    {
        auto                indicesList = m_scene->getIndices();
        aph::BufferLoadInfo loadInfo{
            .data       = indicesList.data(),
            .createInfo = {.size  = static_cast<uint32_t>(indicesList.size() * sizeof(indicesList[0])),
                           .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
            .ppBuffer   = &m_buffers[BUFFER_SCENE_INDEX]};
        m_pResourceLoader->load(loadInfo);
    }

    // create vertex buffer
    {
        auto                verticesList = m_scene->getVertices();
        aph::BufferLoadInfo loadInfo{
            .data       = verticesList.data(),
            .createInfo = {.size  = static_cast<uint32_t>(verticesList.size() * sizeof(verticesList[0])),
                           .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
            .ppBuffer   = &m_buffers[BUFFER_SCENE_VERTEX]};
        m_pResourceLoader->load(loadInfo);
    }

    // create material buffer
    {
        auto                materials = m_scene->getMaterials();
        aph::BufferLoadInfo loadInfo{
            .data       = materials.data(),
            .createInfo = {.size  = static_cast<uint32_t>(materials.size() * sizeof(materials[0])),
                           .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
            .ppBuffer   = &m_buffers[BUFFER_SCENE_MATERIAL]};
        m_pResourceLoader->load(loadInfo);
    }

    // load scene image to gpu
    auto images = m_scene->getImages();
    for(const auto& image : images)
    {
        Image*          texture{};
        ImageCreateInfo createInfo{
            .extent    = {image->width, image->height, 1},
            .mipLevels = aph::utils::calculateFullMipLevels(image->width, image->height),
            .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
            .format    = VK_FORMAT_R8G8B8A8_UNORM,
            .tiling    = VK_IMAGE_TILING_OPTIMAL,
        };

        aph::ImageLoadInfo loadInfo{
            .data          = aph::asset::GetTextureDir() / "container2.png",
            .containerType = aph::ImageContainerType::Png,
            .pCreateInfo   = &createInfo,
            .ppImage       = &texture,
        };
        m_pResourceLoader->load(loadInfo);
        m_images[IMAGE_SCENE_TEXTURES].push_back(texture);
    }

    // create skybox cubemap
    {
        auto skyboxDir    = asset::GetTextureDir() / "skybox";
        auto skyboxImages = aph::utils::loadSkyboxFromFile({
            (skyboxDir / "front.jpg").string(),
            (skyboxDir / "back.jpg").c_str(),
            (skyboxDir / "top_rotate_left_90.jpg").c_str(),
            (skyboxDir / "bottom_rotate_right_90.jpg").c_str(),
            (skyboxDir / "left.jpg").c_str(),
            (skyboxDir / "right.jpg").c_str(),
        });

        Image* pImage{};
        m_pDevice->createCubeMap(skyboxImages, &pImage);
        m_images[IMAGE_SCENE_SKYBOX].push_back(pImage);
    }

    // indirect cmds
    {
        // std::vector<VkDrawIndexedIndirectCommand> drawIndexList;
        std::vector<VkDrawIndirectCommand>     drawList;
        std::vector<VkDispatchIndirectCommand> dispatchList;
        dispatchList.push_back({
            .x = m_wsi->getWidth() / 12,
            .y = m_wsi->getHeight() / 12,
            .z = 1,
        });
        drawList.push_back({
            .vertexCount   = 36,
            .instanceCount = 1,
            .firstVertex   = 0,
            .firstInstance = 0,
        });

        aph::BufferLoadInfo loadInfo{
            .data       = dispatchList.data(),
            .createInfo = {.size  = static_cast<uint32_t>(dispatchList.size() * sizeof(dispatchList[0])),
                           .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
            .ppBuffer   = &m_buffers[BUFFER_INDIRECT_DISPATCH_CMD]};
        m_pResourceLoader->load(loadInfo);

        loadInfo.createInfo.size = static_cast<uint32_t>(drawList.size() * sizeof(drawList[0]));
        loadInfo.ppBuffer        = &m_buffers[BUFFER_INDIRECT_DRAW_CMD];
        loadInfo.data            = drawList.data();
        m_pResourceLoader->load(loadInfo);
    }

    {
        // Create sampler
        SamplerCreateInfo samplerInfo = init::samplerCreateInfo2(SamplerPreset::Linear);
        if(m_pDevice->getFeatures().samplerAnisotropy)
        {
            samplerInfo.maxAnisotropy = m_pDevice->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy;
        }
        VK_CHECK_RESULT(m_pDevice->create(samplerInfo, &m_samplers[SAMP_CUBEMAP]));
        VK_CHECK_RESULT(m_pDevice->create(samplerInfo, &m_samplers[SAMP_SHADOW]));

        samplerInfo.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod     = aph::utils::calculateFullMipLevels(2048, 2048);
        VK_CHECK_RESULT(m_pDevice->create(samplerInfo, &m_samplers[SAMP_TEXTURE]));
    }
}

void SceneRenderer::_initSkybox()
{
    VK_LOG_DEBUG("Init skybox pass.");
    // skybox vertex
    {
        constexpr std::array skyboxVertices = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                                               1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                                               -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                                               -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                                               1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                                               1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                                               -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                                               1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                                               -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                                               1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                                               -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                                               1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
        // create vertex buffer
        {
            aph::BufferLoadInfo loadInfo{
                .data       = (void*)skyboxVertices.data(),
                .createInfo = {.size  = static_cast<uint32_t>(skyboxVertices.size() * sizeof(skyboxVertices[0])),
                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
                .ppBuffer   = &m_buffers[BUFFER_CUBE_VERTEX]};
            m_pResourceLoader->load(loadInfo);
        }
    }

    // TODO skybox graphics pipeline
    {
        // GraphicsPipelineCreateInfo createInfo{{VertexComponent::POSITION}};
        // auto                       shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        // std::vector<VkFormat>      colorFormats = {m_pSwapChain->getFormat()};

        // createInfo.renderingCreateInfo = {
        //     .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        //     .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
        //     .pColorAttachmentFormats = colorFormats.data(),
        //     .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        // };
        // createInfo.multisampling.rasterizationSamples = m_sampleCount;
        // createInfo.multisampling.sampleShadingEnable  = VK_TRUE;
        // createInfo.multisampling.minSampleShading     = 0.2f;

        // createInfo.depthStencil = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);

        // {
        //     auto& program = m_programs[SHADER_PROGRAM_SKYBOX];
        //     VK_CHECK_RESULT(m_pDevice->createShaderProgram(&program, getShaders(shaderDir / "skybox.vert"),
        //                                                    getShaders(shaderDir / "skybox.frag")));
        //     createInfo.pProgram = program;
        //     VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(createInfo, &m_pipelines[PIPELINE_GRAPHICS_SKYBOX]));
        // }
    }
}

void SceneRenderer::recordDeferredLighting(CommandBuffer* pCommandBuffer)
{
    VkExtent2D extent{
        .width  = getWindowWidth(),
        .height = getWindowHeight(),
    };
    VkViewport viewport = init::viewport(extent);
    VkRect2D   scissor  = init::rect2D(extent);

    // dynamic state
    pCommandBuffer->setViewport(viewport);
    pCommandBuffer->setScissor(scissor);

    Image* positionAttachment = m_images[IMAGE_GBUFFER_POSITION][m_frameIdx];
    Image* normalAttachment   = m_images[IMAGE_GBUFFER_NORMAL][m_frameIdx];
    Image* albedoAttachment   = m_images[IMAGE_GBUFFER_ALBEDO][m_frameIdx];
    Image* mraoAttachment     = m_images[IMAGE_GBUFFER_MRAO][m_frameIdx];
    Image* emissiveAttachment = m_images[IMAGE_GBUFFER_EMISSIVE][m_frameIdx];
    Image* pShadowMap         = m_images[IMAGE_SHADOW_DEPTH][m_frameIdx];
    Image* pColorAttachment   = m_images[IMAGE_GENERAL_COLOR][m_frameIdx];
    Image* pDepthAttachment   = m_images[IMAGE_GENERAL_DEPTH][m_frameIdx];

    {
        pCommandBuffer->transitionImageLayout(positionAttachment, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->transitionImageLayout(normalAttachment, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->transitionImageLayout(albedoAttachment, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->transitionImageLayout(mraoAttachment, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->transitionImageLayout(emissiveAttachment, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->transitionImageLayout(pShadowMap, VK_IMAGE_LAYOUT_GENERAL);
    }

    // deferred rendering pass
    {
        pCommandBuffer->beginRendering({.offset{0, 0}, .extent{extent}}, {pColorAttachment}, pDepthAttachment);

        // skybox
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_SKYBOX]);
            pCommandBuffer->bindDescriptorSet({m_skyboxSet, m_samplerSet});
            pCommandBuffer->bindVertexBuffers(m_buffers[BUFFER_CUBE_VERTEX]);
            pCommandBuffer->draw(m_buffers[BUFFER_INDIRECT_DRAW_CMD], 0);
        }

        // draw scene object
        {
            pCommandBuffer->bindVertexBuffers(m_buffers[BUFFER_SCENE_VERTEX]);
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_LIGHTING]);
            pCommandBuffer->bindDescriptorSet({m_sceneSet, m_samplerSet, m_gbufferSets[m_frameIdx]});
            pCommandBuffer->draw(3, 1, 0, 0);
        }

        // draw ui
        recordUIDraw(pCommandBuffer);

        pCommandBuffer->endRendering();
    }
}

void SceneRenderer::recordDeferredGeometry(CommandBuffer* pCommandBuffer)
{
    VkExtent2D extent{
        .width  = getWindowWidth(),
        .height = getWindowHeight(),
    };
    VkViewport viewport = init::viewport(extent);
    VkRect2D   scissor  = init::rect2D(extent);

    // dynamic state
    pCommandBuffer->setViewport(viewport);
    pCommandBuffer->setScissor(scissor);

    // geometry pass
    {
        Image* positionAttachment = m_images[IMAGE_GBUFFER_POSITION][m_frameIdx];
        Image* normalAttachment   = m_images[IMAGE_GBUFFER_NORMAL][m_frameIdx];
        Image* albedoAttachment   = m_images[IMAGE_GBUFFER_ALBEDO][m_frameIdx];
        Image* mraoAttachment     = m_images[IMAGE_GBUFFER_MRAO][m_frameIdx];
        Image* emissiveAttachment = m_images[IMAGE_GBUFFER_EMISSIVE][m_frameIdx];
        Image* depthAttachment    = m_images[IMAGE_GBUFFER_DEPTH][m_frameIdx];

        pCommandBuffer->beginRendering(
            {.offset{0, 0}, .extent{extent}},
            {positionAttachment, normalAttachment, albedoAttachment, mraoAttachment, emissiveAttachment},
            depthAttachment);

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_GEOMETRY]);
            pCommandBuffer->bindDescriptorSet({m_geometrySet, m_samplerSet});
            pCommandBuffer->bindVertexBuffers(m_buffers[BUFFER_SCENE_VERTEX]);
            pCommandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX]);

            for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
            {
                const auto& node = m_meshNodeList[nodeId];
                Mesh*       mesh = node->getObject<Mesh>();
                pCommandBuffer->pushConstants(offsetof(ObjectInfo, nodeId), sizeof(ObjectInfo::nodeId), &nodeId);
                for(const auto& subset : mesh->m_subsets)
                {
                    pCommandBuffer->pushConstants(offsetof(ObjectInfo, materialId), sizeof(ObjectInfo::materialId),
                                                  &subset.materialIndex);
                    if(subset.indexCount > 0)
                    {
                        if(subset.hasIndices)
                        {
                            pCommandBuffer->drawIndexed(subset.indexCount, 1, mesh->m_indexOffset + subset.firstIndex,
                                                        mesh->m_vertexOffset, 0);
                        }
                        else
                        {
                            pCommandBuffer->draw(subset.vertexCount, 1, subset.firstVertex, 0);
                        }
                    }
                }
            }
        }

        pCommandBuffer->endRendering();
    }
}

void SceneRenderer::recordShadow(CommandBuffer* pCommandBuffer)
{
    VkExtent2D extent{4096, 4096};

    // dynamic state
    pCommandBuffer->setViewport(extent);
    pCommandBuffer->setScissor(extent);

    // shadow pass
    {
        Image* pDepthAttachment = m_images[IMAGE_SHADOW_DEPTH][m_frameIdx];
        pCommandBuffer->beginRendering({.offset{0, 0}, .extent{extent}}, {},
                                       {.image = pDepthAttachment, .storeOp = VK_ATTACHMENT_STORE_OP_STORE});

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_SHADOW]);
            pCommandBuffer->bindDescriptorSet({m_shadowSet});
            pCommandBuffer->bindVertexBuffers(m_buffers[BUFFER_SCENE_VERTEX]);
            pCommandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX]);

            for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
            {
                const auto& node = m_meshNodeList[nodeId];
                auto        mesh = node->getObject<Mesh>();
                pCommandBuffer->pushConstants(offsetof(ObjectInfo, nodeId), sizeof(ObjectInfo::nodeId), &nodeId);
                for(const auto& subset : mesh->m_subsets)
                {
                    if(subset.indexCount > 0)
                    {
                        if(subset.hasIndices)
                        {
                            pCommandBuffer->drawIndexed(subset.indexCount, 1, mesh->m_indexOffset + subset.firstIndex,
                                                        mesh->m_vertexOffset, 0);
                        }
                        else
                        {
                            pCommandBuffer->draw(subset.vertexCount, 1, subset.firstVertex, 0);
                        }
                    }
                }
            }
        }

        pCommandBuffer->endRendering();
    }
}

void SceneRenderer::recordPostFX(CommandBuffer* pCommandBuffer)
{
    // post fx
    {
        Image* inputImage  = m_images[IMAGE_GENERAL_COLOR][m_frameIdx];
        Image* outputImage = m_pSwapChain->getImage();

        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView   = m_images[IMAGE_GENERAL_COLOR][m_frameIdx]->getView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo outputImageInfo{.imageView   = m_pSwapChain->getImage()->getView()->getHandle(),
                                                  .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            std::vector<VkWriteDescriptorSet> writes = {
                init::writeDescriptorSet(m_postFxSets[m_frameIdx], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0,
                                         &inputImageInfo),
                init::writeDescriptorSet(m_postFxSets[m_frameIdx], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
                                         &outputImageInfo),
            };
            vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
        }

        pCommandBuffer->transitionImageLayout(inputImage, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->transitionImageLayout(outputImage, VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);
        pCommandBuffer->bindDescriptorSet({m_postFxSets[m_frameIdx]});
        pCommandBuffer->dispatch(m_buffers[BUFFER_INDIRECT_DISPATCH_CMD]);
        pCommandBuffer->transitionImageLayout(outputImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
}

void SceneRenderer::drawUI(float deltaTime)
{
    ImGui::NewFrame();
    ui::drawWindow("Aphrodite - Info", {10, 10}, {0, 0}, m_ui.scale, [&]() {
        ui::text("%s", m_pDevice->getPhysicalDevice()->getProperties().deviceName);
        ui::text("%.2f ms/frame (%.1d fps)", (1000.0f / m_lastFPS), m_lastFPS);
        ui::text("resolution [ %.2f, %.2f ]", (float)m_wsi->getWidth(), (float)m_wsi->getHeight());
        ui::drawWithItemWidth(110.0f, m_ui.scale, [&]() {
            if(ui::header("Scene"))
            {
                {
                    auto ambient = m_scene->getAmbient();
                    ui::colorPicker("ambient", &ambient[0]);
                    m_scene->setAmbient(ambient);
                    ui::text("camera count : %d", m_cameraList.size());
                    ui::text("light count : %d", m_lightList.size());
                }

                if(ui::header("Main Camera"))
                {
                    auto camera  = m_scene->getMainCamera();
                    auto camType = camera->m_cameraType;

                    if(camType == CameraType::PERSPECTIVE)
                    {
                        // ui::text("position : [ %.2f, %.2f, %.2f ]", camera->m_view[3][0],
                        //                     camera->m_view[3][1], camera->m_view[3][2]);
                        ui::sliderFloat("fov", &camera->m_perspective.fov, 30.0f, 120.0f);
                        ui::sliderFloat("znear", &camera->m_perspective.znear, 0.01f, 60.0f);
                        ui::sliderFloat("zfar", &camera->m_perspective.zfar, 60.0f, 200.0f);
                        // ui::checkBox("flipY", &camera->m_flipY);
                        // ui::sliderFloat("rotation speed", &camera->m_rotationSpeed, 0.1f, 1.0f);
                        // ui::sliderFloat("move speed", &camera->m_movementSpeed, 0.1f, 5.0f);
                    }
                    else if(camType == CameraType::ORTHO)
                    {
                        CM_LOG_ERR("TODO");
                        APH_ASSERT(false);
                        // auto camera = m_scene->getMainCamera();
                        // ui::text("position : [ %.2f, %.2f, %.2f ]", camera->m_position.x,
                        //                     camera->m_position.y, camera->m_position.z);
                        // ui::text("rotation : [ %.2f, %.2f, %.2f ]", camera->m_rotation.x,
                        //                     camera->m_rotation.y, camera->m_rotation.z);
                        // ui::checkBox("flipY", &camera->m_flipY);
                        // ui::sliderFloat("rotation speed", &camera->m_rotationSpeed, 0.1f, 1.0f);
                        // ui::sliderFloat("move speed", &camera->m_movementSpeed, 0.1f, 5.0f);
                    }
                }

                for(uint32_t idx = 0; idx < m_lightList.size(); idx++)
                {
                    char lightName[100];
                    sprintf(lightName, "light [%d]", idx);
                    if(ui::header(lightName))
                    {
                        auto light = m_lightList[idx];
                        auto type  = light->m_type;
                        if(type == LightType::POINT)
                        {
                            ui::text("type : point");
                            ImGui::SliderFloat3("position", &light->m_position[0], 0.0f, 1.0f);
                        }
                        else if(type == LightType::DIRECTIONAL)
                        {
                            ui::text("type : directional");
                            ImGui::SliderFloat3("direction", &light->m_direction[0], 0.0f, 1.0f);
                        }
                        ImGui::SliderFloat3("color", &light->m_color[0], 0.0f, 1.0f);
                        ImGui::SliderFloat("intensity", &light->m_intensity, 0.0f, 10.0f);
                    }
                }
            }
        });
    });

    ImGui::Render();
}

void SceneRenderer::_initShadow()
{
    VK_LOG_DEBUG("Init shadow pass.");
    m_images[IMAGE_SHADOW_DEPTH].resize(m_config.maxFrames);
    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        auto&           depth = m_images[IMAGE_SHADOW_DEPTH][idx];
        ImageCreateInfo createInfo{
            // TODO depth map size
            .extent    = {4096, 4096, 1},
            .usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .domain    = ImageDomain::Device,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = m_pDevice->getDepthFormat(),
        };
        VK_CHECK_RESULT(m_pDevice->create(createInfo, &depth));
        m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](auto* cmd) {
            cmd->transitionImageLayout(depth, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        });
    }

    // TODO shadow pipeline
    {
        // GraphicsPipelineCreateInfo createInfo{};

        // auto shaderDir                 = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        // createInfo.renderingCreateInfo = {
        //     .sType                 = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        //     .depthAttachmentFormat = m_pDevice->getDepthFormat(),
        // };

        // createInfo.rasterizer.cullMode        = VK_CULL_MODE_FRONT_BIT;
        // createInfo.rasterizer.depthBiasEnable = VK_TRUE;
        // createInfo.colorBlendAttachments.resize(1, {.blendEnable = VK_FALSE, .colorWriteMask = 0xf});
        // createInfo.depthStencil =
        //     init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        // {
        //     auto& program = m_programs[SHADER_PROGRAM_SHADOW];
        //     VK_CHECK_RESULT(
        //         m_pDevice->createShaderProgram(&program, getShaders(shaderDir / "shadow.vert"), (Shader*)nullptr));
        //     createInfo.pProgram = program;
        //     VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(createInfo, &m_pipelines[PIPELINE_GRAPHICS_SHADOW]));
        // }
    }
}
}  // namespace aph::vk
