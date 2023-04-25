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

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace aph
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
}  // namespace aph

namespace aph
{
VulkanSceneRenderer::VulkanSceneRenderer(std::shared_ptr<Window> window, const RenderConfig& config) :
    VulkanRenderer(std::move(window), config)
{
}

void VulkanSceneRenderer::load(Scene* scene)
{
    m_scene = scene;

    _loadScene();
    _initGpuResources();

    _initSetLayout();
    _initSet();

    _initForward();
    _initSkybox();

    _initPipeline();
}

void VulkanSceneRenderer::cleanup()
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

void VulkanSceneRenderer::recordDrawSceneCommands()
{
    auto* commandBuffer = m_commandBuffers[m_frameIdx];

    commandBuffer->begin();

    recordDrawSceneCommands(commandBuffer);
    recordPostFxCommands(commandBuffer);

    commandBuffer->end();

    QueueSubmitInfo submitInfo{
        .commandBuffers   = {m_commandBuffers[m_frameIdx]},
        .waitStages       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .waitSemaphores   = {m_renderSemaphore[m_frameIdx]},
        .signalSemaphores = {m_presentSemaphore[m_frameIdx]},
    };

    auto* queue = getGraphicsQueue();
    VK_CHECK_RESULT(queue->submit({submitInfo}, m_frameFences[m_frameIdx]));
}

void VulkanSceneRenderer::update(float deltaTime)
{
    m_scene->update(deltaTime);

    {
        SceneInfo sceneInfo = {
            .ambient     = glm::vec4(m_scene->getAmbient(), 0.0f),
            .cameraCount = static_cast<uint32_t>(m_cameraNodeList.size()),
            .lightCount  = static_cast<uint32_t>(m_lightNodeList.size()),
        };
        m_buffers[BUFFER_SCENE_INFO]->write(&sceneInfo, 0, sizeof(SceneInfo));
    }

    for(uint32_t idx = 0; idx < m_meshNodeList.size(); idx++)
    {
        const auto& node = m_meshNodeList[idx];
        auto        data = node->getTransform();
        m_buffers[BUFFER_SCENE_TRANSFORM]->write(&data, sizeof(glm::mat4) * idx, sizeof(glm::mat4));
    }

    for(uint32_t idx = 0; idx < m_cameraNodeList.size(); idx++)
    {
        const auto& camera = m_cameraNodeList[idx]->getObject<Camera>();
        CameraInfo  cameraData{
             .view    = camera->m_view,
             .proj    = camera->m_projection,
             .viewPos = {camera->m_view[3]},
        };
        m_buffers[BUFFER_SCENE_CAMERA]->write(&cameraData, sizeof(CameraInfo) * idx, sizeof(CameraInfo));
    }

    for(uint32_t idx = 0; idx < m_lightNodeList.size(); idx++)
    {
        const auto& light = m_lightNodeList[idx]->getObject<Light>();
        LightInfo   lightData{
              .color     = {light->m_color * light->m_intensity, 1.0f},
              .position  = {light->m_position, 1.0f},
              .direction = {light->m_direction, 1.0f},
              .lightType = utils::getUnderLyingType(light->m_type),
        };
        m_buffers[BUFFER_SCENE_LIGHT]->write(&lightData, sizeof(LightInfo) * idx, sizeof(LightInfo));
    }

    drawUI(deltaTime);
    updateUIDrawData(deltaTime);
}

void VulkanSceneRenderer::_initSet()
{
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

void VulkanSceneRenderer::_loadScene()
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
            m_cameraNodeList.push_back(node);
        }
        break;
        case ObjectType::LIGHT:
        {
            m_lightNodeList.push_back(node);
        }
        break;
        default: assert("unattached scene node."); break;
        }
    });
}

void VulkanSceneRenderer::_initForward()
{
    VkExtent2D imageExtent = {m_pSwapChain->getWidth(), m_pSwapChain->getHeight()};

    m_images[IMAGE_FORWARD_COLOR].resize(m_config.maxFrames);
    m_images[IMAGE_FORWARD_DEPTH].resize(m_config.maxFrames);
    m_images[IMAGE_FORWARD_COLOR_MS].resize(m_config.maxFrames);
    m_images[IMAGE_FORWARD_DEPTH_MS].resize(m_config.maxFrames);

    // frame buffer
    for(auto idx = 0; idx < m_config.maxFrames; idx++)
    {
        {
            auto& colorImage   = m_images[IMAGE_FORWARD_COLOR][idx];
            auto& colorImageMS = m_images[IMAGE_FORWARD_COLOR_MS][idx];

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
            auto&           depthImage   = m_images[IMAGE_FORWARD_DEPTH][idx];
            auto&           depthImageMS = m_images[IMAGE_FORWARD_DEPTH_MS][idx];
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
            m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](VulkanCommandBuffer* cmd) {
                cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                cmd->transitionImageLayout(depthImageMS, VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            });
        }
    }
}

void VulkanSceneRenderer::_initSetLayout()
{
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
        }
        {
            VkSamplerCreateInfo samplerInfo = init::samplerCreateInfo();
            samplerInfo.maxLod              = utils::calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VK_CHECK_RESULT(m_pDevice->createSampler(samplerInfo, &m_samplers[SAMP_TEXTURE]));
        }

        std::vector<ResourcesBinding> bindings{
            {ResourceType::SAMPLER, {ShaderStage::FS}, 1, &m_samplers[SAMP_TEXTURE]},
            {ResourceType::SAMPLER, {ShaderStage::FS}, 1, &m_samplers[SAMP_CUBEMAP]},
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
}

void VulkanSceneRenderer::_initGpuResources()
{
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
            .size   = static_cast<uint32_t>(m_cameraNodeList.size() * sizeof(CameraInfo)),
            .usage  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_CAMERA]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_CAMERA]);
    }

    // create light buffer
    {
        BufferCreateInfo createInfo{
            .size   = static_cast<uint32_t>(m_lightNodeList.size() * sizeof(LightInfo)),
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
            .mipLevels = utils::calculateFullMipLevels(image->width, image->height),
            .usage     = VK_IMAGE_USAGE_SAMPLED_BIT,
            .format    = VK_FORMAT_R8G8B8A8_UNORM,
            .tiling    = VK_IMAGE_TILING_OPTIMAL,
        };

        VulkanImage* texture{};
        m_pDevice->createDeviceLocalImage(createInfo, &texture, image->data);
        m_images[IMAGE_SCENE_TEXTURES].push_back(texture);
    }

    // create skybox cubemap
    {
        auto skyboxDir    = AssetManager::GetTextureDir() / "skybox";
        auto skyboxImages = utils::loadSkyboxFromFile({
            (skyboxDir / "front.jpg").string(),
            (skyboxDir / "back.jpg").c_str(),
            (skyboxDir / "top_rotate_left_90.jpg").c_str(),
            (skyboxDir / "bottom_rotate_right_90.jpg").c_str(),
            (skyboxDir / "left.jpg").c_str(),
            (skyboxDir / "right.jpg").c_str(),
        });

        VulkanImage* pImage{};
        m_pDevice->createCubeMap(skyboxImages, &pImage, &m_pCubeMapView);
        m_images[IMAGE_SCENE_SKYBOX].push_back(pImage);
    }
}

void VulkanSceneRenderer::_initSkybox()
{
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
}

void VulkanSceneRenderer::recordDrawSceneCommands(VulkanCommandBuffer* pCommandBuffer)
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
        VulkanImageView*          pColorAttachment   = m_images[IMAGE_FORWARD_COLOR][m_frameIdx]->getView();
        VulkanImageView*          pColorAttachmentMS = m_images[IMAGE_FORWARD_COLOR_MS][m_frameIdx]->getView();
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

        VulkanImageView*          pDepthAttachment   = m_images[IMAGE_FORWARD_DEPTH][m_frameIdx]->getView();
        VulkanImageView*          pDepthAttachmentMS = m_images[IMAGE_FORWARD_DEPTH_MS][m_frameIdx]->getView();
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
                    case IndexType::UINT16: indexType = VK_INDEX_TYPE_UINT16; break;
                    case IndexType::UINT32: indexType = VK_INDEX_TYPE_UINT32; break;
                    default: assert("undefined behavior."); break;
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
                        else { pCommandBuffer->draw(subset.vertexCount, 1, subset.firstVertex, 0); }
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
}
void VulkanSceneRenderer::recordPostFxCommands(VulkanCommandBuffer* pCommandBuffer)
{
    // post fx
    {
        VulkanImageView* pColorAttachment = m_pSwapChain->getImage()->getView();

        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);

        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView   = m_images[IMAGE_FORWARD_COLOR][m_frameIdx]->getView()->getHandle(),
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

void VulkanSceneRenderer::drawUI(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2(m_window->getWidth(), m_window->getHeight());
    io.DeltaTime   = 1.0f;

    io.AddMousePosEvent((float)m_window->getCursorX(), (float)m_window->getCursorY());
    io.AddMouseButtonEvent(0, m_window->getMouseButtonStatus(input::MOUSE_BUTTON_LEFT) == input::STATUS_PRESS);
    io.AddMouseButtonEvent(1, m_window->getMouseButtonStatus(input::MOUSE_BUTTON_RIGHT) == input::STATUS_PRESS);
    io.AddMouseButtonEvent(2, m_window->getMouseButtonStatus(input::MOUSE_BUTTON_MIDDLE) == input::STATUS_PRESS);

    ImGui::NewFrame();

    ui::drawWindow("Aphrodite - Info", {10, 10}, {0, 0}, m_ui.scale, [&]() {
        ui::text("%s", m_pDevice->getPhysicalDevice()->getProperties().deviceName);
        ui::text("%.2f ms/frame (%.1d fps)", (1000.0f / m_lastFPS), m_lastFPS);
        ui::text("resolution [ %.2f, %.2f ]", (float)m_window->getWidth(), (float)m_window->getHeight());
        ui::drawWithItemWidth(110.0f, m_ui.scale, [&]() {
            if(ui::header("Input"))
            {
                ui::text("cursor pos : [ %.2f, %.2f ]", m_window->getCursorX(), m_window->getCursorY());
                ui::text("cursor visible : %s", m_window->getMouseData()->isCursorVisible ? "yes" : "no");
            }
            if(ui::header("Scene"))
            {
                {
                    auto ambient = m_scene->getAmbient();
                    ui::colorPicker("ambient", &ambient[0]);
                    m_scene->setAmbient(ambient);
                    ui::text("camera count : %d", m_cameraNodeList.size());
                    ui::text("light count : %d", m_lightNodeList.size());
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

                for(uint32_t idx = 0; idx < m_lightNodeList.size(); idx++)
                {
                    char lightName[100];
                    sprintf(lightName, "light [%d]", idx);
                    if(ui::header(lightName))
                    {
                        auto light = m_lightNodeList[idx]->getObject<Light>();
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
void VulkanSceneRenderer::_initPipeline()
{
    // forward graphics pipeline
    {
        GraphicsPipelineCreateInfo createInfo{};

        auto                  shaderDir    = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = {m_pSwapChain->getFormat()};
        createInfo.renderingCreateInfo     = VkPipelineRenderingCreateInfo{
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

    // skybox graphics pipeline
    {
        GraphicsPipelineCreateInfo createInfo{{VertexComponent::POSITION}};
        auto                       shaderDir    = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
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

    // postfx compute pipeline
    {
        std::filesystem::path     shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        ComputePipelineCreateInfo createInfo{};
        createInfo.setLayouts                     = {m_setLayouts[SET_LAYOUT_POSTFX]};
        createInfo.shaderMapList[ShaderStage::CS] = getShaders(shaderDir / "postFX.comp.spv");
        VK_CHECK_RESULT(m_pDevice->createComputePipeline(createInfo, &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
    }
}
}  // namespace aph
