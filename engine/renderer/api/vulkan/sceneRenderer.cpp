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
#include <imgui_impl_glfw.h>

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
SceneRenderer::SceneRenderer(std::shared_ptr<WSI> window, const RenderConfig& config) :
    Renderer(std::move(window), config)
{
}

void SceneRenderer::load(Scene* scene)
{
    m_scene = scene;

    _loadScene();
    _initGpuResources();

    _initSetLayout();
    _initSet();

    _initShadow();
    _initGbuffer();
    _initGeneral();
    _initSkybox();
}

void SceneRenderer::cleanup()
{
    for(auto* pipeline : m_pipelines)
    {
        m_pDevice->destroyPipeline(pipeline);
    }

    for(auto* setLayout : m_setLayouts)
    {
        m_pDevice->destroyDescriptorSetLayout(setLayout);
    }

    m_pDevice->destroyImageView(m_pCubeMapView);

    for(const auto& images : m_images)
    {
        for(auto* image : images)
        {
            m_pDevice->destroyImage(image);
        }
    }

    for(auto* buffer : m_buffers)
    {
        m_pDevice->destroyBuffer(buffer);
    }

    for(auto* const sampler : m_samplers)
    {
        m_pDevice->destroySampler(sampler);
    }
}

void SceneRenderer::recordAll()
{
    auto* queue = getGraphicsQueue();
    enum CBIdx
    {
        GEOMETRY,
        SHADOW,
        LIGHTING,
        POSTFX,
        CB_MAX,
    };
    CommandBuffer* cb[CB_MAX] = {};
    m_pDevice->allocateCommandBuffers(CB_MAX, cb, queue);

    // TODO multi thread
    {
        cb[SHADOW]->begin();
        recordShadow(cb[SHADOW]);
        cb[SHADOW]->end();

        cb[GEOMETRY]->begin();
        recordDeferredGeometry(cb[GEOMETRY]);
        cb[GEOMETRY]->end();

        cb[LIGHTING]->begin();
        recordDeferredLighting(cb[LIGHTING]);
        cb[LIGHTING]->end();

        cb[POSTFX]->begin();
        recordPostFX(cb[POSTFX]);
        cb[POSTFX]->end();
    }

    std::array<std::vector<VkSemaphoreSubmitInfo>, CB_MAX>     waitSemaphoreInfos;
    std::array<std::vector<VkSemaphoreSubmitInfo>, CB_MAX>     signalSemaphoreInfos;
    std::array<VkCommandBufferSubmitInfo, CB_MAX> cbSubmitInfo;
    std::vector<VkSubmitInfo2>                    submitInfos(CB_MAX);

    {
        // timeline
        VkSemaphore& timelineMain = m_timelineMain[m_frameIdx];
        VkSemaphore timelineShadow{};

        m_pSyncPrimitivesPool->acquireTimelineSemaphore(1, &timelineMain);
        m_pSyncPrimitivesPool->acquireTimelineSemaphore(1, &timelineShadow);

        // 1 geometry && shadow
        signalSemaphoreInfos[GEOMETRY].push_back({.semaphore = timelineMain, .value = 1});
        signalSemaphoreInfos[SHADOW].push_back({.semaphore = timelineShadow, .value = 1});

        // 2 lighting
        waitSemaphoreInfos[LIGHTING].push_back({.semaphore = timelineShadow, .value = 1});
        waitSemaphoreInfos[LIGHTING].push_back({.semaphore = timelineMain, .value = 1});

        signalSemaphoreInfos[LIGHTING].push_back({.semaphore = timelineMain, .value = 2});

        // 3 postfx
        waitSemaphoreInfos[POSTFX].push_back({.semaphore = m_renderSemaphore[m_frameIdx]});
        waitSemaphoreInfos[POSTFX].push_back({.semaphore = timelineMain, .value = 2});

        signalSemaphoreInfos[POSTFX].push_back({.semaphore = m_presentSemaphore[m_frameIdx]});
        signalSemaphoreInfos[POSTFX].push_back({.semaphore = timelineMain, .value = UINT64_MAX});
    }

    for(auto idx = 0; idx < CB_MAX; idx++)
    {
        auto& cbSI   = cbSubmitInfo[idx];
        auto& waitSI = waitSemaphoreInfos[idx];
        auto& sigSI  = signalSemaphoreInfos[idx];
        auto& si     = submitInfos[idx];

        {
            cbSI.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            cbSI.commandBuffer = cb[idx]->getHandle();
            cbSI.pNext         = nullptr;
            cbSI.deviceMask    = 0;
        }

        for (auto& wait : waitSI)
        {
            wait.pNext       = nullptr;
            wait.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            wait.stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            wait.deviceIndex = 0;
        }

        for (auto& sig : sigSI)
        {
            sig.pNext       = nullptr;
            sig.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            sig.stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            sig.deviceIndex = 0;
        }

        si.pNext                    = nullptr;
        si.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        si.commandBufferInfoCount   = 1;
        si.pCommandBufferInfos      = &cbSI;
        si.waitSemaphoreInfoCount   = waitSI.size();
        si.pWaitSemaphoreInfos      = waitSI.data();
        si.signalSemaphoreInfoCount = sigSI.size();
        si.pSignalSemaphoreInfos    = sigSI.data();
    }

    VK_CHECK_RESULT(vkQueueSubmit2(queue->getHandle(), submitInfos.size(), submitInfos.data(), VK_NULL_HANDLE));
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
             .viewPos = {camera->m_view[3]},
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
    VK_LOG_INFO("Init descriptor set.");
    VkDescriptorBufferInfo sceneBufferInfo{m_buffers[BUFFER_SCENE_INFO]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo materialBufferInfo{m_buffers[BUFFER_SCENE_MATERIAL]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo cameraBufferInfo{m_buffers[BUFFER_SCENE_CAMERA]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo lightBufferInfo{m_buffers[BUFFER_SCENE_LIGHT]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo transformBufferInfo{m_buffers[BUFFER_SCENE_TRANSFORM]->getHandle(), 0, VK_WHOLE_SIZE};
    VkDescriptorImageInfo  skyBoxImageInfo{nullptr, m_pCubeMapView->getHandle(),
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    std::vector<VkDescriptorImageInfo> textureInfos{};
    for(auto& texture : m_images[IMAGE_SCENE_TEXTURES])
    {
        VkDescriptorImageInfo info{
            .imageView   = texture->getView()->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        textureInfos.push_back(info);
    }

    std::vector<ResourceWrite> writes{
        {{}, &sceneBufferInfo},
        {{}, &transformBufferInfo},
        {{}, &cameraBufferInfo},
        {{}, &lightBufferInfo},
        {textureInfos.data(), {}, textureInfos.size()},
        {{}, &materialBufferInfo},
        {&skyBoxImageInfo, {}},
    };

    m_sceneSet   = m_setLayouts[SET_LAYOUT_SCENE]->allocateSet(writes);
    m_samplerSet = m_setLayouts[SET_LAYOUT_SAMP]->allocateSet();
}

void SceneRenderer::_loadScene()
{
    bool enabledShadow = true;
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
            assert("unattached scene node.");
            break;
        }
    });

    {
        // TODO for testing
        auto* cam = new Camera{CameraType::PERSPECTIVE};
        cam->m_view = glm::lookAt(glm::vec3(1.0f), glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        cam->m_projection = glm::perspective(glm::radians(100.0f), 1.0f, 0.1f, 64.0f);
        m_cameraList.push_back(cam);
    }
}

void SceneRenderer::_initGbuffer()
{
    VK_LOG_INFO("Init deferred pass.");
    VkExtent2D imageExtent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight()};
    m_images[IMAGE_GBUFFER_ALBEDO].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_NORMAL].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_POSITION].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_EMISSIVE].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_METALLIC_ROUGHNESS_AO].resize(m_config.maxFrames);
    m_images[IMAGE_GBUFFER_DEPTH].resize(m_config.maxFrames);

    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        {
            auto& position            = m_images[IMAGE_GBUFFER_POSITION][idx];
            auto& normal              = m_images[IMAGE_GBUFFER_NORMAL][idx];
            auto& albedo              = m_images[IMAGE_GBUFFER_ALBEDO][idx];
            auto& emissive            = m_images[IMAGE_GBUFFER_EMISSIVE][idx];
            auto& metallicRoughnessAO = m_images[IMAGE_GBUFFER_METALLIC_ROUGHNESS_AO][idx];

            ImageCreateInfo createInfo{
                .extent    = {imageExtent.width, imageExtent.height, 1},
                .usage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain    = ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = VK_FORMAT_R16G16B16A16_SFLOAT,
            };
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &position));
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &normal));
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &metallicRoughnessAO));

            createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &albedo));
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &emissive));
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
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depth));
            m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](auto* cmd) {
                cmd->transitionImageLayout(depth, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
            });
        }
    }

    // geometry graphics pipeline
    {
        GraphicsPipelineCreateInfo createInfo{};

        auto                  shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT,
                                              VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT,
                                              VK_FORMAT_R8G8B8A8_UNORM};
        createInfo.renderingCreateInfo     = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
                .pColorAttachmentFormats = colorFormats.data(),
                .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };

        // createInfo.multisampling.rasterizationSamples = m_sampleCount;
        // createInfo.multisampling.sampleShadingEnable  = VK_TRUE;
        // createInfo.multisampling.minSampleShading     = 0.2f;
        createInfo.colorBlendAttachments.resize(5, {.blendEnable = VK_FALSE, .colorWriteMask = 0xf});

        createInfo.setLayouts = {m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP]};
        createInfo.constants  = {{utils::VkCast({ShaderStage::VS, ShaderStage::FS}), 0, sizeof(ObjectInfo)}};
        createInfo.shaderMapList[ShaderStage::VS] = getShaders(shaderDir / "geometry.vert.spv");
        createInfo.shaderMapList[ShaderStage::FS] = getShaders(shaderDir / "geometry.frag.spv");

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(createInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_GEOMETRY]));
    }

    // deferred light pbr pipeline
    {
        GraphicsPipelineCreateInfo createInfo{{}};

        auto                  shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = {m_pSwapChain->getFormat()};
        createInfo.renderingCreateInfo     = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
                .pColorAttachmentFormats = colorFormats.data(),
                .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };

        createInfo.multisampling.rasterizationSamples = m_sampleCount;
        createInfo.multisampling.sampleShadingEnable  = VK_TRUE;
        createInfo.multisampling.minSampleShading     = 0.2f;

        createInfo.setLayouts = {m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP],
                                 m_setLayouts[SET_LAYOUT_GBUFFER]};
        createInfo.constants  = {{utils::VkCast({ShaderStage::VS, ShaderStage::FS}), 0, sizeof(ObjectInfo)}};
        createInfo.shaderMapList[ShaderStage::VS] = getShaders(shaderDir / "pbr_deferred.vert.spv");
        createInfo.shaderMapList[ShaderStage::FS] = getShaders(shaderDir / "pbr_deferred.frag.spv");

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(createInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_LIGHTING]));
    }
}

void SceneRenderer::_initGeneral()
{
    VK_LOG_INFO("Init general pass.");
    VkExtent2D imageExtent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight()};

    m_images[IMAGE_GENERAL_COLOR].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_DEPTH].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_COLOR_MS].resize(m_config.maxFrames);
    m_images[IMAGE_GENERAL_DEPTH_MS].resize(m_config.maxFrames);

    // frame buffer
    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        {
            auto& colorImage   = m_images[IMAGE_GENERAL_COLOR][idx];
            auto& colorImageMS = m_images[IMAGE_GENERAL_COLOR_MS][idx];

            ImageCreateInfo createInfo{
                .extent = {imageExtent.width, imageExtent.height, 1},
                .usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .domain = ImageDomain::Device,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = VK_FORMAT_B8G8R8A8_UNORM,
            };
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &colorImage));
            createInfo.samples = m_sampleCount;
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &colorImageMS));
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
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depthImage));
            createInfo.samples = m_sampleCount;
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depthImageMS));

            m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](auto* cmd) {
                cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
                cmd->transitionImageLayout(depthImageMS, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
            });
        }
    }

    // postfx compute pipeline
    {
        std::filesystem::path     shaderDir = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        ComputePipelineCreateInfo createInfo{};
        createInfo.setLayouts                     = {m_setLayouts[SET_LAYOUT_POSTFX]};
        createInfo.shaderMapList[ShaderStage::CS] = getShaders(shaderDir / "postFX.comp.spv");
        VK_CHECK_RESULT(m_pDevice->createComputePipeline(createInfo, &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
    }
}

void SceneRenderer::_initSetLayout()
{
    VK_LOG_INFO("Init descriptor set.");
    // scene
    {
        std::vector<ResourcesBinding> bindings{
            {ResourceType::UNIFORM_BUFFER, {ShaderStage::VS, ShaderStage::FS}},
            {ResourceType::UNIFORM_BUFFER, {ShaderStage::VS, ShaderStage::FS}},
            {ResourceType::UNIFORM_BUFFER, {ShaderStage::VS, ShaderStage::FS}},
            {ResourceType::UNIFORM_BUFFER, {ShaderStage::VS, ShaderStage::FS}},
            {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}, m_images[IMAGE_SCENE_TEXTURES].size()},
            {ResourceType::UNIFORM_BUFFER, {ShaderStage::FS}},
            {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}},
        };
        m_pDevice->createDescriptorSetLayout(bindings, &m_setLayouts[SET_LAYOUT_SCENE]);
    }

    // sampler
    {
        {
            // Create sampler
            VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo();
            if(m_pDevice->getFeatures().samplerAnisotropy)
            {
                samplerInfo.maxAnisotropy = m_pDevice->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy;
                samplerInfo.anisotropyEnable = VK_TRUE;
            }
            VK_CHECK_RESULT(m_pDevice->createSampler(samplerInfo, &m_samplers[SAMP_CUBEMAP]));
            VK_CHECK_RESULT(m_pDevice->createSampler(samplerInfo, &m_samplers[SAMP_SHADOW]));

            samplerInfo.maxLod      = aph::utils::calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VK_CHECK_RESULT(m_pDevice->createSampler(samplerInfo, &m_samplers[SAMP_TEXTURE]));
        }

        std::vector<ResourcesBinding> bindings{
            {ResourceType::SAMPLER, {ShaderStage::FS}, 1, &m_samplers[SAMP_TEXTURE]},
            {ResourceType::SAMPLER, {ShaderStage::FS}, 1, &m_samplers[SAMP_CUBEMAP]},
            {ResourceType::SAMPLER, {ShaderStage::FS}, 1, &m_samplers[SAMP_SHADOW]},
        };
        m_pDevice->createDescriptorSetLayout(bindings, &m_setLayouts[SET_LAYOUT_SAMP]);
    }

    // off screen texture
    {
        std::vector<ResourcesBinding> bindings{
            {ResourceType::STORAGE_IMAGE, {ShaderStage::CS}},
            {ResourceType::STORAGE_IMAGE, {ShaderStage::CS}},
        };
        m_pDevice->createDescriptorSetLayout(bindings, &m_setLayouts[SET_LAYOUT_POSTFX], true);
    }

    {
        std::vector<ResourcesBinding> bindings{
            {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}}, {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}},
            {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}}, {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}},
            {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}},
            {ResourceType::SAMPLED_IMAGE, {ShaderStage::FS}},
        };
        m_pDevice->createDescriptorSetLayout(bindings, &m_setLayouts[SET_LAYOUT_GBUFFER], true);
    }
}

void SceneRenderer::_initGpuResources()
{
    VK_LOG_INFO("Init GPU resources.");
    // create scene info buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(sizeof(SceneInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_INFO]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_INFO]);
    }
    // create camera buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(m_cameraList.size() * sizeof(CameraInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_CAMERA]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_CAMERA]);
    }

    // create light buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(m_lightList.size() * sizeof(LightInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_LIGHT]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_LIGHT]);
    }

    // create transform buffer
    {
        BufferCreateInfo createInfo{.size   = static_cast<uint32_t>(m_meshNodeList.size() * sizeof(glm::mat4)),
                                    .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    .domain = BufferDomain::Host};
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_TRANSFORM]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_TRANSFORM]);
    }

    // create index buffer
    {
        auto             indicesList = m_scene->getIndices();
        BufferCreateInfo createInfo{
            .size  = static_cast<uint32_t>(indicesList.size()),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_INDEX], indicesList.data());
    }

    // create vertex buffer
    {
        auto             verticesList = m_scene->getVertices();
        BufferCreateInfo createInfo{
            .size  = static_cast<uint32_t>(verticesList.size()),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_VERTEX], verticesList.data());
    }

    // create material buffer
    {
        auto             materials = m_scene->getMaterials();
        BufferCreateInfo createInfo{
            .size  = static_cast<uint32_t>(materials.size() * sizeof(Material)),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_MATERIAL], materials.data());
    }

    // load scene image to gpu
    auto images = m_scene->getImages();
    for(const auto& image : images)
    {
        ImageCreateInfo createInfo{
            .extent    = {image->width, image->height, 1},
            .mipLevels = aph::utils::calculateFullMipLevels(image->width, image->height),
            .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
            .format    = VK_FORMAT_R8G8B8A8_UNORM,
            .tiling    = VK_IMAGE_TILING_OPTIMAL,
        };

        Image* texture{};
        m_pDevice->createDeviceLocalImage(createInfo, &texture, image->data);
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
        m_pDevice->createCubeMap(skyboxImages, &pImage, &m_pCubeMapView);
        m_images[IMAGE_SCENE_SKYBOX].push_back(pImage);
    }
}

void SceneRenderer::_initSkybox()
{
    VK_LOG_INFO("Init skybox pass.");
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
            BufferCreateInfo createInfo{
                .size  = static_cast<uint32_t>(skyboxVertices.size() * sizeof(float)),
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            };
            m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_CUBE_VERTEX], skyboxVertices.data());
        }
    }

    // skybox graphics pipeline
    {
        GraphicsPipelineCreateInfo createInfo{{VertexComponent::POSITION}};
        auto                       shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        std::vector<VkFormat>      colorFormats = {m_pSwapChain->getFormat()};

        createInfo.renderingCreateInfo = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };
        createInfo.multisampling.rasterizationSamples = m_sampleCount;
        createInfo.multisampling.sampleShadingEnable  = VK_TRUE;
        createInfo.multisampling.minSampleShading     = 0.2f;

        createInfo.depthStencil = init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);
        createInfo.setLayouts   = {m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP]};
        createInfo.shaderMapList[ShaderStage::VS] = getShaders(shaderDir / "skybox.vert.spv");
        createInfo.shaderMapList[ShaderStage::FS] = getShaders(shaderDir / "skybox.frag.spv");

        VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(createInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_SKYBOX]));
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
    pCommandBuffer->setSissor(scissor);

    // forward pass
    {
        ImageView*                pColorAttachment = m_images[IMAGE_GENERAL_COLOR][m_frameIdx]->getView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };

        ImageView*                pDepthAttachment = m_images[IMAGE_GENERAL_DEPTH][m_frameIdx]->getView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = pDepthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue  = {.depthStencil{1.0f, 0}},
        };
        if(m_sampleCount == VK_SAMPLE_COUNT_1_BIT)
        {
            forwardDepthAttachmentInfo.imageView   = pDepthAttachment->getHandle();
            forwardDepthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        }

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{0, 0},
                .extent{extent},
            },
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &forwardColorAttachmentInfo,
            .pDepthAttachment     = &forwardDepthAttachmentInfo,
        };

        {
            pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        pCommandBuffer->beginRendering(renderingInfo);

        // skybox
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_SKYBOX]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SKYBOX], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SKYBOX], 1, 1, &m_samplerSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_CUBE_VERTEX], {0});
            pCommandBuffer->draw(36, 1, 0, 0);
        }

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_LIGHTING]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_LIGHTING], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_LIGHTING], 1, 1, &m_samplerSet);

            {
                VkDescriptorImageInfo posImageInfo{
                    .imageView   = m_images[IMAGE_GBUFFER_POSITION][m_frameIdx]->getView()->getHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
                VkDescriptorImageInfo normalImageInfo{
                    .imageView   = m_images[IMAGE_GBUFFER_NORMAL][m_frameIdx]->getView()->getHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
                VkDescriptorImageInfo albedoImageInfo{
                    .imageView   = m_images[IMAGE_GBUFFER_ALBEDO][m_frameIdx]->getView()->getHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
                VkDescriptorImageInfo metallicRoughnessAOImageInfo{
                    .imageView   = m_images[IMAGE_GBUFFER_METALLIC_ROUGHNESS_AO][m_frameIdx]->getView()->getHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
                VkDescriptorImageInfo emissiveImageInfo{
                    .imageView   = m_images[IMAGE_GBUFFER_EMISSIVE][m_frameIdx]->getView()->getHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
                VkDescriptorImageInfo shadowMapInfo{
                    .imageView   = m_images[IMAGE_SHADOW_DEPTH][m_frameIdx]->getView()->getHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

                std::vector<VkWriteDescriptorSet> writes{
                    init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, &posImageInfo),
                    init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, &normalImageInfo),
                    init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2, &albedoImageInfo),
                    init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3,
                                             &metallicRoughnessAOImageInfo),
                    init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4, &emissiveImageInfo),
                    init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5, &shadowMapInfo),
                };
                pCommandBuffer->pushDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_LIGHTING], writes, 2);
            }

            pCommandBuffer->draw(3, 1, 0, 0);
        }

        // draw ui
        recordUIDraw(pCommandBuffer);

        pCommandBuffer->endRendering();

        {
            pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        }
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
    pCommandBuffer->setSissor(scissor);

    // geometry pass
    {
        ImageView* positionAttachment            = m_images[IMAGE_GBUFFER_POSITION][m_frameIdx]->getView();
        ImageView* normalAttachment              = m_images[IMAGE_GBUFFER_NORMAL][m_frameIdx]->getView();
        ImageView* albedoAttachment              = m_images[IMAGE_GBUFFER_ALBEDO][m_frameIdx]->getView();
        ImageView* emissiveAttachment            = m_images[IMAGE_GBUFFER_EMISSIVE][m_frameIdx]->getView();
        ImageView* metallicRoughnessAOAttachment = m_images[IMAGE_GBUFFER_METALLIC_ROUGHNESS_AO][m_frameIdx]->getView();

        // 0 pos, 1 normal, 2 albedo
        std::array<VkRenderingAttachmentInfo, 5> colorAttachments;
        colorAttachments[0] = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = positionAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };
        colorAttachments[1] = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = normalAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };
        colorAttachments[2] = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = albedoAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };
        colorAttachments[3] = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = metallicRoughnessAOAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };
        colorAttachments[4] = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = emissiveAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };

        ImageView*                depthAttachment = m_images[IMAGE_GBUFFER_DEPTH][m_frameIdx]->getView();
        VkRenderingAttachmentInfo depthAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = depthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue  = {.depthStencil{1.0f, 0}},
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{0, 0},
                .extent{extent},
            },
            .layerCount           = 1,
            .colorAttachmentCount = colorAttachments.size(),
            .pColorAttachments    = colorAttachments.data(),
            .pDepthAttachment     = &depthAttachmentInfo,
        };

        {
            pCommandBuffer->transitionImageLayout(positionAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            pCommandBuffer->transitionImageLayout(normalAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            pCommandBuffer->transitionImageLayout(albedoAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            pCommandBuffer->transitionImageLayout(emissiveAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            pCommandBuffer->transitionImageLayout(metallicRoughnessAOAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        pCommandBuffer->beginRendering(renderingInfo);

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_GEOMETRY]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_GEOMETRY], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_GEOMETRY], 1, 1, &m_samplerSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_SCENE_VERTEX], {0});

            for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
            {
                const auto& node = m_meshNodeList[nodeId];
                Mesh*       mesh = node->getObject<Mesh>();
                pCommandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_GEOMETRY],
                                              {ShaderStage::VS, ShaderStage::FS}, offsetof(ObjectInfo, nodeId),
                                              sizeof(ObjectInfo::nodeId), &nodeId);
                if(mesh->m_indexOffset > -1)
                {
                    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
                    switch(mesh->m_indexType)
                    {
                    case IndexType::UINT16:
                        indexType = VK_INDEX_TYPE_UINT16;
                        break;
                    case IndexType::UINT32:
                        indexType = VK_INDEX_TYPE_UINT32;
                        break;
                    default:
                        assert("undefined behavior.");
                        break;
                    }
                    pCommandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX], 0, indexType);
                }
                for(const auto& subset : mesh->m_subsets)
                {
                    pCommandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_GEOMETRY],
                                                  {ShaderStage::VS, ShaderStage::FS}, offsetof(ObjectInfo, materialId),
                                                  sizeof(ObjectInfo::materialId), &subset.materialIndex);
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

        {
            pCommandBuffer->transitionImageLayout(positionAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            pCommandBuffer->transitionImageLayout(normalAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            pCommandBuffer->transitionImageLayout(albedoAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            pCommandBuffer->transitionImageLayout(emissiveAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            pCommandBuffer->transitionImageLayout(metallicRoughnessAOAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        }
    }

}

void SceneRenderer::recordShadow(CommandBuffer* pCommandBuffer)
{
    // TODO
    VkExtent2D extent{.width  = 4096, .height = 4096,};
    VkViewport viewport = init::viewport(extent);
    VkRect2D   scissor  = init::rect2D(extent);

    // dynamic state
    pCommandBuffer->setViewport(viewport);
    pCommandBuffer->setSissor(scissor);

    // forward pass
    {
        ImageView*                pDepthAttachment   = m_images[IMAGE_SHADOW_DEPTH][m_frameIdx]->getView();
        VkRenderingAttachmentInfo depthAttachmentInfo{
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView          = pDepthAttachment->getHandle(),
            .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue         = {.depthStencil{1.0f, 0}},
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{0, 0},
                .extent{extent},
            },
            .layerCount           = 1,
            .pDepthAttachment     = &depthAttachmentInfo,
        };

        {
            pCommandBuffer->transitionImageLayout(pDepthAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        }

        pCommandBuffer->beginRendering(renderingInfo);

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_SHADOW]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SHADOW], 0, 1, &m_sceneSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_SCENE_VERTEX], {0});

            for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
            {
                const auto& node = m_meshNodeList[nodeId];
                auto        mesh = node->getObject<Mesh>();
                pCommandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_SHADOW],
                                              {ShaderStage::VS}, offsetof(ObjectInfo, nodeId),
                                              sizeof(ObjectInfo::nodeId), &nodeId);
                if(mesh->m_indexOffset > -1)
                {
                    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
                    switch(mesh->m_indexType)
                    {
                    case IndexType::UINT16:
                        indexType = VK_INDEX_TYPE_UINT16;
                        break;
                    case IndexType::UINT32:
                        indexType = VK_INDEX_TYPE_UINT32;
                        break;
                    default:
                        assert("undefined behavior.");
                        break;
                    }
                    pCommandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX], 0, indexType);
                }
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

        {
            pCommandBuffer->transitionImageLayout(pDepthAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        }
    }
}

void SceneRenderer::recordForward(CommandBuffer* pCommandBuffer)
{
    VkExtent2D extent{
        .width  = getWindowWidth(),
        .height = getWindowHeight(),
    };
    VkViewport viewport = init::viewport(extent);
    VkRect2D   scissor  = init::rect2D(extent);

    // dynamic state
    pCommandBuffer->setViewport(viewport);
    pCommandBuffer->setSissor(scissor);

    // forward pass
    {
        ImageView*                pColorAttachment   = m_images[IMAGE_GENERAL_COLOR][m_frameIdx]->getView();
        ImageView*                pColorAttachmentMS = m_images[IMAGE_GENERAL_COLOR_MS][m_frameIdx]->getView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView          = pColorAttachmentMS->getHandle(),
            .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT,
            .resolveImageView   = pColorAttachment->getHandle(),
            .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue         = {.color{{0.1f, 0.1f, 0.1f, 1.0f}}},
        };

        if(m_sampleCount == VK_SAMPLE_COUNT_1_BIT)
        {
            forwardColorAttachmentInfo.imageView   = pColorAttachment->getHandle();
            forwardColorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        }

        ImageView*                pDepthAttachment   = m_images[IMAGE_GENERAL_DEPTH][m_frameIdx]->getView();
        ImageView*                pDepthAttachmentMS = m_images[IMAGE_GENERAL_DEPTH_MS][m_frameIdx]->getView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView          = pDepthAttachmentMS->getHandle(),
            .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT,
            .resolveImageView   = pDepthAttachment->getHandle(),
            .resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp            = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue         = {.depthStencil{1.0f, 0}},
        };
        if(m_sampleCount == VK_SAMPLE_COUNT_1_BIT)
        {
            forwardDepthAttachmentInfo.imageView   = pDepthAttachment->getHandle();
            forwardDepthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
        }

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{0, 0},
                .extent{extent},
            },
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &forwardColorAttachmentInfo,
            .pDepthAttachment     = &forwardDepthAttachmentInfo,
        };

        {
            pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            pCommandBuffer->transitionImageLayout(pColorAttachmentMS->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        pCommandBuffer->beginRendering(renderingInfo);

        // skybox
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_SKYBOX]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SKYBOX], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SKYBOX], 1, 1, &m_samplerSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_CUBE_VERTEX], {0});
            pCommandBuffer->draw(36, 1, 0, 0);
        }

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_FORWARD]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 1, 1, &m_samplerSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_SCENE_VERTEX], {0});

            for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
            {
                const auto& node = m_meshNodeList[nodeId];
                auto        mesh = node->getObject<Mesh>();
                pCommandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
                                              {ShaderStage::VS, ShaderStage::FS}, offsetof(ObjectInfo, nodeId),
                                              sizeof(ObjectInfo::nodeId), &nodeId);
                if(mesh->m_indexOffset > -1)
                {
                    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
                    switch(mesh->m_indexType)
                    {
                    case IndexType::UINT16:
                        indexType = VK_INDEX_TYPE_UINT16;
                        break;
                    case IndexType::UINT32:
                        indexType = VK_INDEX_TYPE_UINT32;
                        break;
                    default:
                        assert("undefined behavior.");
                        break;
                    }
                    pCommandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX], 0, indexType);
                }
                for(const auto& subset : mesh->m_subsets)
                {
                    if(subset.indexCount > 0)
                    {
                        pCommandBuffer->pushConstants(
                            m_pipelines[PIPELINE_GRAPHICS_FORWARD], {ShaderStage::VS, ShaderStage::FS},
                            offsetof(ObjectInfo, materialId), sizeof(ObjectInfo::materialId), &subset.materialIndex);
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

        // draw ui
        recordUIDraw(pCommandBuffer);

        pCommandBuffer->endRendering();

        {
            pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            pCommandBuffer->transitionImageLayout(pColorAttachmentMS->getImage(),
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        }
    }

    // forward graphics pipeline
    {
        GraphicsPipelineCreateInfo createInfo{};

        auto                  shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = {m_pSwapChain->getFormat()};
        createInfo.renderingCreateInfo     = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
                .pColorAttachmentFormats = colorFormats.data(),
                .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };

        createInfo.multisampling.rasterizationSamples = m_sampleCount;
        createInfo.multisampling.sampleShadingEnable  = VK_TRUE;
        createInfo.multisampling.minSampleShading     = 0.2f;

        createInfo.setLayouts = {m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP]};
        createInfo.constants  = {{utils::VkCast({ShaderStage::VS, ShaderStage::FS}), 0, sizeof(ObjectInfo)}};
        createInfo.shaderMapList[ShaderStage::VS] = getShaders(shaderDir / "pbr.vert.spv");
        createInfo.shaderMapList[ShaderStage::FS] = getShaders(shaderDir / "pbr.frag.spv");

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(createInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_FORWARD]));
    }
}

void SceneRenderer::recordPostFX(CommandBuffer* pCommandBuffer)
{
    // post fx
    {
        ImageView* pColorAttachment = m_pSwapChain->getImage()->getView();

        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);

        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView   = m_images[IMAGE_GENERAL_COLOR][m_frameIdx]->getView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo outputImageInfo{.imageView   = pColorAttachment->getHandle(),
                                                  .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

            std::vector<VkWriteDescriptorSet> writes{
                init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &inputImageInfo),
                init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &outputImageInfo),
            };

            pCommandBuffer->pushDescriptorSet(m_pipelines[PIPELINE_COMPUTE_POSTFX], writes, 0);
        }

        pCommandBuffer->dispatch(pColorAttachment->getImage()->getWidth(), pColorAttachment->getImage()->getHeight(),
                                 1);
        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_GENERAL,
                                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
}

void SceneRenderer::drawUI(float deltaTime)
{
    ImGui::NewFrame();
    ui::drawWindow("Aphrodite - Info", {10, 10}, {0, 0}, m_ui.scale, [&]() {
        ui::text("%s", m_pDevice->getPhysicalDevice()->getProperties().deviceName);
        ui::text("%.2f ms/frame (%.1d fps)", (1000.0f / m_lastFPS), m_lastFPS);
        ui::text("resolution [ %.2f, %.2f ]", (float)m_window->getWidth(), (float)m_window->getHeight());
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
                        assert("TODO");
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
    VK_LOG_INFO("Init shadow pass.");
    m_images[IMAGE_SHADOW_DEPTH].resize(m_config.maxFrames);
    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        auto &depth = m_images[IMAGE_SHADOW_DEPTH][idx];
        ImageCreateInfo createInfo{
            // TODO depth map size
            .extent    = {4096, 4096, 1},
            .usage     = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .domain    = ImageDomain::Device,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = m_pDevice->getDepthFormat(),
        };
        VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depth));
        m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](auto* cmd) {
            cmd->transitionImageLayout(depth, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        });
    }

    // shadow pipeline
    {
        GraphicsPipelineCreateInfo createInfo{};

        auto                  shaderDir    = asset::GetShaderDir(asset::ShaderType::GLSL) / "default";
        createInfo.renderingCreateInfo     = {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };

        createInfo.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        createInfo.rasterizer.depthBiasEnable = VK_TRUE;
        createInfo.colorBlendAttachments.resize(1, {.blendEnable = VK_FALSE, .colorWriteMask = 0xf});
        createInfo.depthStencil = init::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        createInfo.setLayouts = {m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP]};
        createInfo.constants  = {{utils::VkCast(ShaderStage::VS), 0, sizeof(ObjectInfo)}};
        createInfo.shaderMapList[ShaderStage::VS] = getShaders(shaderDir / "shadow.vert.spv");

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(createInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_SHADOW]));
    }
}
}  // namespace aph::vk
