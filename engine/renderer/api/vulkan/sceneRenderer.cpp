#include "sceneRenderer.h"

#include "common/assetManager.h"

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"

#include "api/vulkan/device.h"

#include <glm/gtx/string_cast.hpp>
#include <utility>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace aph
{
VulkanSceneRenderer::VulkanSceneRenderer(std::shared_ptr<Window> window, const RenderConfig& config) :
    VulkanRenderer(std::move(window), config)
{
}

void VulkanSceneRenderer::loadResources()
{
    _loadScene();
    _initGpuResources();

    _initSetLayout();
    _initSet();

    _initForward();
    _initSkybox();
    _initPostFx();
}

void VulkanSceneRenderer::cleanupResources()
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

    for(const auto sampler : m_samplers)
    {
        vkDestroySampler(m_pDevice->getHandle(), sampler, nullptr);
    }
}

void VulkanSceneRenderer::recordDrawSceneCommands()
{
    uint32_t frameIdx      = getCurrentFrameIndex();
    auto*    commandBuffer = getDefaultCommandBuffer(frameIdx);

    commandBuffer->begin();

    recordDrawSceneCommands(commandBuffer);
    recordPostFxCommands(commandBuffer);

    commandBuffer->end();
}

void VulkanSceneRenderer::update(float deltaTime)
{
    for(uint32_t idx = 0; idx < m_meshNodeList.size(); idx++)
    {
        const auto& node      = m_meshNodeList[idx];
        m_transformInfos[idx] = node->getTransform();
    }
    m_buffers[BUFFER_SCENE_TRANSFORM]->copyTo(m_transformInfos.data(), 0, m_buffers[BUFFER_SCENE_TRANSFORM]->getSize());

    for(uint32_t idx = 0; idx < m_cameraNodeList.size(); idx++)
    {
        const auto& camera = m_cameraNodeList[idx]->getObject<Camera>();
        camera->processMovement(deltaTime);
        CameraInfo cameraData{
            .view    = camera->getViewMatrix(),
            .proj    = camera->getProjMatrix(),
            .viewPos = camera->getPosition(),
        };
        m_buffers[BUFFER_SCENE_CAMERA]->copyTo(&cameraData, sizeof(CameraInfo) * idx, sizeof(CameraInfo));
    }

    for(uint32_t idx = 0; idx < m_lightNodeList.size(); idx++)
    {
        const auto& light = m_lightNodeList[idx]->getObject<Light>();
        LightInfo   lightData{
              .color     = light->getColor(),
              .position  = light->getPosition(),
              .direction = light->getDirection(),
        };
        m_buffers[BUFFER_SCENE_LIGHT]->copyTo(&lightData, sizeof(LightInfo) * idx, sizeof(LightInfo));
    }

    _updateUI();
}

void VulkanSceneRenderer::_initSet()
{
    m_samplerSet = m_setLayouts[SET_LAYOUT_SAMP]->allocateSet();
    m_sceneSet   = m_setLayouts[SET_LAYOUT_SCENE]->allocateSet();

    m_sceneInfo = {
        .ambient     = glm::vec4(m_scene->getAmbient(), 0.0f),
        .cameraCount = static_cast<uint32_t>(m_cameraNodeList.size()),
        .lightCount  = static_cast<uint32_t>(m_lightNodeList.size()),
    };

    VkWriteDescriptorSetInlineUniformBlock writeDescriptorSetInlineUniformBlock{
        .sType    = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT,
        .dataSize = sizeof(SceneInfo),
        .pData    = &m_sceneInfo,
    };

    VkWriteDescriptorSet sceneInfoSetWrite{
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext           = &writeDescriptorSetInlineUniformBlock,
        .dstSet          = m_sceneSet,
        .dstBinding      = 0,
        .descriptorCount = sizeof(SceneInfo),
        .descriptorType  = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
    };

    std::vector<VkDescriptorImageInfo> textureInfos{};
    for(auto& texture : m_images[IMAGE_SCENE_TEXTURES])
    {
        VkDescriptorImageInfo info{
            .imageView   = texture->getImageView()->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        textureInfos.push_back(info);
    }

    VkDescriptorImageInfo skyBoxInfo{
        .sampler     = nullptr,
        .imageView   = m_pCubeMapView->getHandle(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorBufferInfo materialBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_MATERIAL]->getHandle(),
                                               .offset = 0,
                                               .range  = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo cameraBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_CAMERA]->getHandle(),
                                             .offset = 0,
                                             .range  = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo lightBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_LIGHT]->getHandle(),
                                            .offset = 0,
                                            .range  = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo transformBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_TRANSFORM]->getHandle(),
                                                .offset = 0,
                                                .range  = VK_WHOLE_SIZE };

    std::vector<VkWriteDescriptorSet> writes{
        sceneInfoSetWrite,
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &transformBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &cameraBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &lightBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4, textureInfos.data(),
                                      textureInfos.size()),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, &materialBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 6, &skyBoxInfo, 1),
    };
    vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
}

void VulkanSceneRenderer::_loadScene()
{
    std::queue<std::shared_ptr<SceneNode>> q;
    q.push(m_scene->getRootNode());

    while(!q.empty())
    {
        const auto node = q.front();
        q.pop();

        switch(node->getAttachType())
        {
        case ObjectType::MESH:
        {
            m_transformInfos.push_back(node->getTransform());
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
        default:
            assert("unattached scene node.");
            break;
        }

        for(const auto& subNode : node->getChildren())
        {
            q.push(subNode);
        }
    }
}

void VulkanSceneRenderer::_initPostFx()
{
    // build pipeline
    std::filesystem::path     shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
    ComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.setLayouts    = { m_setLayouts[SET_LAYOUT_POSTFX] };
    pipelineCreateInfo.shaderMapList = {
        { VK_SHADER_STAGE_COMPUTE_BIT, getShaderCache()->getShaders(shaderDir / "postFX.comp.spv") },
    };
    VK_CHECK_RESULT(m_pDevice->createComputePipeline(pipelineCreateInfo, &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
}

void VulkanSceneRenderer::_initForward()
{
    uint32_t   imageCount  = getSwapChain()->getImageCount();
    VkExtent2D imageExtent = getSwapChain()->getExtent();

    m_images[IMAGE_FORWARD_COLOR].resize(imageCount);
    m_images[IMAGE_FORWARD_DEPTH].resize(imageCount);

    // frame buffer
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto& colorImage = m_images[IMAGE_FORWARD_COLOR][idx];
        auto& depthImage = m_images[IMAGE_FORWARD_DEPTH][idx];

        {
            ImageCreateInfo createInfo{
                .extent    = { imageExtent.width, imageExtent.height, 1 },
                .usage     = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_STORAGE_BIT | IMAGE_USAGE_SAMPLED_BIT,
                .property  = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .imageType = ImageType::_2D,
                .format    = Format::B8G8R8A8_UNORM,
            };
            m_pDevice->createImage(createInfo, &colorImage);
        }

        {
            ImageCreateInfo createInfo{
                .extent   = { imageExtent.width, imageExtent.height, 1 },
                .usage    = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format   = static_cast<Format>(m_pDevice->getDepthFormat()),
                .tiling   = ImageTiling::OPTIMAL,
            };
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depthImage));
        }

        m_pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer* cmd) {
            cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        });
    }

    // forward graphics pipeline
    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        auto                       shaderDir    = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat>      colorFormats = { getSwapChain()->getSurfaceFormat() };
        pipelineCreateInfo.renderingCreateInfo  = VkPipelineRenderingCreateInfo{
             .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
             .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
             .pColorAttachmentFormats = colorFormats.data(),
             .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP] };
        pipelineCreateInfo.constants.push_back(aph::init::pushConstantRange(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectInfo), 0));
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, getShaderCache()->getShaders(shaderDir / "pbr.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, getShaderCache()->getShaders(shaderDir / "pbr.frag.spv") },
        };

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_FORWARD]));
    }
}

void VulkanSceneRenderer::_initSetLayout()
{
    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                                  sizeof(SceneInfo)),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 4,
                                                  m_images[IMAGE_SCENE_TEXTURES].size()),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 6),
        };

        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_SCENE]);
    }

    // sampler
    {
        {
            // Create sampler
            VkSamplerCreateInfo samplerInfo = aph::init::samplerCreateInfo();
            samplerInfo.magFilter           = VK_FILTER_LINEAR;
            samplerInfo.minFilter           = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV        = samplerInfo.addressModeU;
            samplerInfo.addressModeW        = samplerInfo.addressModeU;
            samplerInfo.mipLodBias          = 0.0f;
            samplerInfo.compareOp           = VK_COMPARE_OP_NEVER;
            samplerInfo.minLod              = 0.0f;
            // samplerInfo.maxLod              = aph::utils::calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerInfo.maxAnisotropy = 1.0f;
            // if (m_pDevice->features.samplerAnisotropy)
            // {
            //     sampler.maxAnisotropy = 100;
            //     sampler.anisotropyEnable = VK_TRUE;
            // }
            VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &samplerInfo, nullptr, &m_samplers[SAMP_CUBEMAP]));
        }
        {
            VkSamplerCreateInfo samplerInfo = aph::init::samplerCreateInfo();
            samplerInfo.maxLod              = aph::utils::calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &samplerInfo, nullptr, &m_samplers[SAMP_TEXTURE]));
        }
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1,
                                                  &m_samplers[SAMP_TEXTURE]),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1,
                                                  &m_samplers[SAMP_CUBEMAP]),
        };

        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_SAMP]);
    }

    // off screen texture
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
        };

        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        createInfo.flags                           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_POSTFX]);
    }
}

void VulkanSceneRenderer::_initGpuResources()
{
    // create camera buffer
    {
        BufferCreateInfo createInfo{
            .size     = static_cast<uint32_t>(m_cameraNodeList.size() * sizeof(CameraInfo)),
            .usage    = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_CAMERA]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_CAMERA]);
    }

    // create light buffer
    {
        BufferCreateInfo createInfo{
            .size     = static_cast<uint32_t>(m_lightNodeList.size() * sizeof(LightInfo)),
            .usage    = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_LIGHT]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_LIGHT]);
    }

    // create transform buffer
    {
        BufferCreateInfo createInfo{
            .size     = static_cast<uint32_t>(m_meshNodeList.size() * sizeof(glm::mat4)),
            .usage    = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_TRANSFORM], m_transformInfos.data(), true);
    }

    // create index buffer
    {
        auto             indicesList = m_scene->getIndices();
        BufferCreateInfo createInfo{
            .size  = static_cast<uint32_t>(indicesList.size()),
            .usage = BUFFER_USAGE_INDEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_INDEX], indicesList.data());
    }

    // create vertex buffer
    {
        auto             verticesList = m_scene->getVertices();
        BufferCreateInfo createInfo{
            .size  = static_cast<uint32_t>(verticesList.size()),
            .usage = BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_VERTEX], verticesList.data());
    }

    // create material buffer
    {
        auto             materials = m_scene->getMaterials();
        BufferCreateInfo createInfo{
            .size  = static_cast<uint32_t>(materials.size() * sizeof(Material)),
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_MATERIAL], materials.data());
    }

    // load scene image to gpu
    auto images = m_scene->getImages();
    for(const auto& image : images)
    {
        ImageCreateInfo createInfo{
            .extent    = { image->width, image->height, 1 },
            .mipLevels = aph::utils::calculateFullMipLevels(image->width, image->height),
            .usage     = IMAGE_USAGE_SAMPLED_BIT,
            .format    = Format::R8G8B8A8_UNORM,
            .tiling    = ImageTiling::OPTIMAL,
        };

        VulkanImage* texture{};
        m_pDevice->createDeviceLocalImage(createInfo, &texture, image->data);
        m_images[IMAGE_SCENE_TEXTURES].push_back(texture);
    }

    // create skybox cubemap
    {
        auto skyboxDir    = AssetManager::GetTextureDir() / "skybox";
        auto skyboxImages = aph::utils::loadSkyboxFromFile({
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
        constexpr std::array skyboxVertices = { -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
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
                                                1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f };
        // create vertex buffer
        {
            BufferCreateInfo createInfo{
                .size  = static_cast<uint32_t>(skyboxVertices.size() * sizeof(float)),
                .usage = BUFFER_USAGE_VERTEX_BUFFER_BIT,
            };
            m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_CUBE_VERTEX], skyboxVertices.data());
        }
    }

    // skybox graphics pipeline
    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{ { VertexComponent::POSITION } };
        auto                       shaderDir    = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat>      colorFormats = { getSwapChain()->getSurfaceFormat() };
        pipelineCreateInfo.renderingCreateInfo  = {
             .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
             .colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size()),
             .pColorAttachmentFormats = colorFormats.data(),
             .depthAttachmentFormat   = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.depthStencil =
            aph::init::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS);
        pipelineCreateInfo.setLayouts    = { m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP] };
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, getShaderCache()->getShaders(shaderDir / "skybox.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, getShaderCache()->getShaders(shaderDir / "skybox.frag.spv") },
        };

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_SKYBOX]));
    }
}

void VulkanSceneRenderer::recordDrawSceneCommands(VulkanCommandBuffer* pCommandBuffer)
{
    uint32_t imageIdx = getCurrentImageIndex();

    VkExtent2D extent{
        .width  = getWindowWidth(),
        .height = getWindowHeight(),
    };
    VkViewport viewport = aph::init::viewport(extent);
    VkRect2D   scissor  = aph::init::rect2D(extent);

    // dynamic state
    pCommandBuffer->setViewport(viewport);
    pCommandBuffer->setSissor(scissor);

    // forward pass
    {
        VulkanImageView*          pColorAttachment = m_images[IMAGE_FORWARD_COLOR][imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = { .color{ { 0.1f, 0.1f, 0.1f, 1.0f } } },
        };

        VulkanImageView*          pDepthAttachment = m_images[IMAGE_FORWARD_DEPTH][imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = pDepthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue  = { .depthStencil{ 1.0f, 0 } },
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{ 0, 0 },
                .extent{ extent },
            },
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &forwardColorAttachmentInfo,
            .pDepthAttachment     = &forwardDepthAttachmentInfo,
        };

        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        pCommandBuffer->transitionImageLayout(pDepthAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        pCommandBuffer->beginRendering(renderingInfo);

        // skybox
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_SKYBOX]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SKYBOX], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_SKYBOX], 1, 1, &m_samplerSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_CUBE_VERTEX], { 0 });
            pCommandBuffer->draw(36, 1, 0, 0);
        }

        // draw scene object
        {
            pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_FORWARD]);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 0, 1, &m_sceneSet);
            pCommandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 1, 1, &m_samplerSet);
            pCommandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_SCENE_VERTEX], { 0 });

            for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
            {
                const auto& node = m_meshNodeList[nodeId];
                auto        mesh = node->getObject<Mesh>();
                pCommandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
                                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                              offsetof(ObjectInfo, nodeId), sizeof(ObjectInfo::nodeId), &nodeId);
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
                        pCommandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
                                                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                      offsetof(ObjectInfo, materialId), sizeof(ObjectInfo::materialId),
                                                      &subset.materialIndex);
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
        {
            if(m_pUIRenderer)
            {
                m_pUIRenderer->draw(pCommandBuffer);
            }
        }

        pCommandBuffer->endRendering();

        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                              VK_IMAGE_LAYOUT_GENERAL);
    }
}
void VulkanSceneRenderer::recordPostFxCommands(VulkanCommandBuffer* pCommandBuffer)
{
    uint32_t imageIdx = getCurrentImageIndex();
    // post fx
    {
        VulkanImageView* pColorAttachment = getSwapChain()->getImage(imageIdx)->getImageView();

        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_GENERAL);
        pCommandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);

        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView   = m_images[IMAGE_FORWARD_COLOR][imageIdx]->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkDescriptorImageInfo outputImageInfo{
                .imageView   = getSwapChain()->getImage(imageIdx)->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            std::vector<VkWriteDescriptorSet> writes{
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &inputImageInfo),
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &outputImageInfo),
            };

            pCommandBuffer->pushDescriptorSet(m_pipelines[PIPELINE_COMPUTE_POSTFX], writes, 0);
        }

        pCommandBuffer->dispatch(pColorAttachment->getImage()->getWidth(), pColorAttachment->getImage()->getHeight(),
                                 1);
        pCommandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_GENERAL,
                                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
}

void VulkanSceneRenderer::_updateUI()
{
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)m_window->getWidth(), (float)m_window->getHeight());
    io.DeltaTime   = 1.0f;

    io.MousePos = ImVec2(m_window->getCursorXpos(), m_window->getCursorYpos());
    // io.MouseDown[0] = mouseButtons.left;
    // io.MouseDown[1] = mouseButtons.right;
    // io.MouseDown[2] = mouseButtons.middle;

    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(10 * m_pUIRenderer->getScaleFactor(), 10 * m_pUIRenderer->getScaleFactor()));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Aphrodite - Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(m_pDevice->getPhysicalDevice()->getProperties().deviceName);
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / m_lastFPS), m_lastFPS);

    ImGui::PushItemWidth(110.0f * m_pUIRenderer->getScaleFactor());
    m_pUIRenderer->header("Scene Info");
    m_pUIRenderer->text("ambient: [%.2f, %.2f, %.2f]", m_sceneInfo.ambient.x, m_sceneInfo.ambient.y, m_sceneInfo.ambient.z);
    m_pUIRenderer->text("camera count : %d", m_sceneInfo.cameraCount);
    m_pUIRenderer->text("light count : %d", m_sceneInfo.lightCount);
    m_pUIRenderer->header("Main Camera Info");
    m_pUIRenderer->text("position : [%.2f, %.2f, %.2f]", m_sceneInfo.ambient.x, m_sceneInfo.ambient.y, m_sceneInfo.ambient.z);
    m_pUIRenderer->text("camera count : %d", m_sceneInfo.cameraCount);
    m_pUIRenderer->text("light count : %d", m_sceneInfo.lightCount);
    ImGui::PopItemWidth();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::Render();
}
}  // namespace aph
