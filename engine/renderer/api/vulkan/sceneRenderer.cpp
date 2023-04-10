#include "sceneRenderer.h"

#include "common/assetManager.h"

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"

#include "api/vulkan/device.h"

#include <glm/gtx/string_cast.hpp>

namespace aph
{

struct SceneInfo
{
    glm::vec4 ambient{ 0.04f };
    uint32_t cameraCount{};
    uint32_t lightCount{};
};

struct CameraInfo
{
    glm::mat4 view{ 1.0f };
    glm::mat4 proj{ 1.0f };
    glm::vec3 viewPos{ 1.0f };
};

struct LightInfo
{
    glm::vec3 color{ 1.0f };
    glm::vec3 position{ 1.0f };
    glm::vec3 direction{ 1.0f };
};

struct ObjectInfo
{
    uint32_t nodeId{};
    uint32_t materialId{};
};

}  // namespace aph

namespace aph
{
VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer) :
    m_pDevice{ renderer->getDevice() },
    m_pRenderer{ renderer }
{
}

void VulkanSceneRenderer::loadResources()
{
    _loadScene();
    _initGpuResources();

    _initSetLayout();
    _initSet();

    _initForward();
    _initPostFx();
}

void VulkanSceneRenderer::cleanupResources()
{
    for(auto *pipeline : m_pipelines)
    {
        m_pDevice->destroyPipeline(pipeline);
    }

    for(auto *setLayout : m_setLayouts)
    {
        m_pDevice->destroyDescriptorSetLayout(setLayout);
    }

    for(const auto &images : m_images)
    {
        for(auto *image : images)
        {
            m_pDevice->destroyImage(image);
        }
    }

    for(auto *buffer : m_buffers)
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
    VkExtent2D extent{
        .width = m_pRenderer->getWindowWidth(),
        .height = m_pRenderer->getWindowHeight(),
    };
    VkViewport viewport = aph::init::viewport(extent);
    VkRect2D scissor = aph::init::rect2D(extent);

    VkCommandBufferBeginInfo beginInfo = aph::init::commandBufferBeginInfo();

    uint32_t imageIdx = m_pRenderer->getCurrentImageIndex();
    uint32_t frameIdx = m_pRenderer->getCurrentFrameIndex();

    auto *commandBuffer = m_pRenderer->getDefaultCommandBuffer(frameIdx);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->setViewport(viewport);
    commandBuffer->setSissor(scissor);

    // forward pass
    {
        VulkanImageView *pColorAttachment = m_images[IMAGE_FORWARD_COLOR][imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color{ { 0.1f, 0.1f, 0.1f, 1.0f } } },
        };

        VulkanImageView *pDepthAttachment = m_images[IMAGE_FORWARD_DEPTH][imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pDepthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = { .depthStencil{ 1.0f, 0 } },
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{ 0, 0 },
                .extent{ extent },
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &forwardColorAttachmentInfo,
            .pDepthAttachment = &forwardDepthAttachmentInfo,
        };

        commandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_FORWARD]);
        commandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 0, 1, &m_sceneSet);
        commandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 1, 1, &m_samplerSet);

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        commandBuffer->transitionImageLayout(pDepthAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        commandBuffer->beginRendering(renderingInfo);
        commandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_SCENE_VERTEX], { 0 });

        for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
        {
            const auto &node = m_meshNodeList[nodeId];
            auto mesh = node->getObject<Mesh>();
            commandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
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
                commandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX], 0, indexType);
            }
            for(const auto &subset : mesh->m_subsets)
            {
                if(subset.indexCount > 0)
                {
                    commandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
                                                 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                 offsetof(ObjectInfo, materialId), sizeof(ObjectInfo::materialId),
                                                 &subset.materialIndex);
                    if(subset.hasIndices)
                    {
                        commandBuffer->drawIndexed(subset.indexCount, 1, mesh->m_indexOffset + subset.firstIndex,
                                                   mesh->m_vertexOffset, 0);
                    }
                    else
                    {
                        commandBuffer->draw(subset.vertexCount, 1, subset.firstVertex, 0);
                    }
                }
            }
        }
        commandBuffer->endRendering();

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                             VK_IMAGE_LAYOUT_GENERAL);
    }

    // post fx
    {
        VulkanImageView *pColorAttachment = m_pRenderer->getSwapChain()->getImage(imageIdx)->getImageView();

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL);
        commandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);

        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView = m_images[IMAGE_FORWARD_COLOR][imageIdx]->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkDescriptorImageInfo outputImageInfo{
                .imageView = m_pRenderer->getSwapChain()->getImage(imageIdx)->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            std::vector<VkWriteDescriptorSet> writes{
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &inputImageInfo),
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &outputImageInfo),
            };

            commandBuffer->pushDescriptorSet(m_pipelines[PIPELINE_COMPUTE_POSTFX], writes, 0);
        }

        commandBuffer->dispatch(pColorAttachment->getImage()->getWidth(), pColorAttachment->getImage()->getHeight(), 1);
        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_GENERAL,
                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    commandBuffer->end();
}

void VulkanSceneRenderer::update(float deltaTime)
{
    for(uint32_t idx = 0; idx < m_meshNodeList.size(); idx++)
    {
        const auto &node = m_meshNodeList[idx];
        m_transformInfos[idx] = node->getTransform();
    }
    m_buffers[BUFFER_SCENE_TRANSFORM]->copyTo(m_transformInfos.data(), 0, m_buffers[BUFFER_SCENE_TRANSFORM]->getSize());

    for(uint32_t idx = 0; idx < m_cameraNodeList.size(); idx++)
    {
        const auto &camera = m_cameraNodeList[idx]->getObject<Camera>();
        camera->processMovement(deltaTime);
        CameraInfo cameraData{
            .view = camera->getViewMatrix(),
            .proj = camera->getProjMatrix(),
            .viewPos = camera->getPosition(),
        };
        m_buffers[BUFFER_SCENE_CAMERA]->copyTo(&cameraData, sizeof(CameraInfo) * idx, sizeof(CameraInfo));
    }

    for(uint32_t idx = 0; idx < m_lightNodeList.size(); idx++)
    {
        const auto &light = m_lightNodeList[idx]->getObject<Light>();
        LightInfo lightData{
            .color = light->getColor(),
            .position = light->getPosition(),
            .direction = light->getDirection(),
        };
        m_buffers[BUFFER_SCENE_LIGHT]->copyTo(&lightData, sizeof(LightInfo) * idx, sizeof(LightInfo));
    }
}

void VulkanSceneRenderer::_initSet()
{
    m_samplerSet = m_setLayouts[SET_LAYOUT_SAMP]->allocateSet();
    m_sceneSet = m_setLayouts[SET_LAYOUT_SCENE]->allocateSet();

    SceneInfo info{
        .ambient = glm::vec4(m_scene->getAmbient(), 0.0f),
        .cameraCount = static_cast<uint32_t>(m_cameraNodeList.size()),
        .lightCount = static_cast<uint32_t>(m_lightNodeList.size()),
    };

    VkWriteDescriptorSetInlineUniformBlock writeDescriptorSetInlineUniformBlock{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT,
        .dataSize = sizeof(SceneInfo),
        .pData = &info,
    };

    VkWriteDescriptorSet sceneInfoSetWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &writeDescriptorSetInlineUniformBlock,
        .dstSet = m_sceneSet,
        .dstBinding = 0,
        .descriptorCount = sizeof(SceneInfo),
        .descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
    };

    std::vector<VkDescriptorImageInfo> textureInfos{};
    for(auto &texture : m_images[IMAGE_SCENE_TEXTURES])
    {
        VkDescriptorImageInfo info{
            .imageView = texture->getImageView()->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        textureInfos.push_back(info);
    }

    VkDescriptorImageInfo skyBoxInfo{
        .sampler = nullptr,
        .imageView = m_cubeMapView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorBufferInfo materialBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_MATERIAL]->getHandle(),
                                               .offset = 0,
                                               .range = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo cameraBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_CAMERA]->getHandle(),
                                             .offset = 0,
                                             .range = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo lightBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_LIGHT]->getHandle(),
                                            .offset = 0,
                                            .range = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo transformBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_TRANSFORM]->getHandle(),
                                                .offset = 0,
                                                .range = VK_WHOLE_SIZE };

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

        for(const auto &subNode : node->getChildren())
        {
            q.push(subNode);
        }
    }
}

void VulkanSceneRenderer::_initPostFx()
{
    // build pipeline
    std::filesystem::path shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
    ComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_POSTFX] };
    pipelineCreateInfo.shaderMapList = {
        { VK_SHADER_STAGE_COMPUTE_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "postFX.comp.spv") },
    };
    VK_CHECK_RESULT(m_pDevice->createComputePipeline(pipelineCreateInfo, &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
}

void VulkanSceneRenderer::_initForward()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    VkExtent2D imageExtent = m_pRenderer->getSwapChain()->getExtent();

    m_images[IMAGE_FORWARD_COLOR].resize(imageCount);
    m_images[IMAGE_FORWARD_DEPTH].resize(imageCount);

    // frame buffer
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto &colorImage = m_images[IMAGE_FORWARD_COLOR][idx];
        auto &depthImage = m_images[IMAGE_FORWARD_DEPTH][idx];

        {
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .imageType = IMAGE_TYPE_2D,
                .usage = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_STORAGE_BIT | IMAGE_USAGE_SAMPLED_BIT,
                .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImage(createInfo, &colorImage);
        }

        {
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = static_cast<Format>(m_pDevice->getDepthFormat()),
                .tiling = IMAGE_TILING_OPTIMAL,
            };
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depthImage));
        }

        m_pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *cmd) {
            cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        });
    }

    // forward graphics pipeline
    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = { m_pRenderer->getSwapChain()->getSurfaceFormat() };
        pipelineCreateInfo.renderingCreateInfo = VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_SCENE], m_setLayouts[SET_LAYOUT_SAMP] };
        pipelineCreateInfo.constants.push_back(aph::init::pushConstantRange(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectInfo), 0));
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv") },
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
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = samplerInfo.addressModeU;
            samplerInfo.addressModeW = samplerInfo.addressModeU;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
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
            samplerInfo.maxLod = calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
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
        createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_POSTFX]);
    }
}

void VulkanSceneRenderer::_initGpuResources()
{
    // create camera buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_cameraNodeList.size() * sizeof(CameraInfo)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_CAMERA]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_CAMERA]);
    }

    // create light buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_lightNodeList.size() * sizeof(LightInfo)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_LIGHT]);
        m_pDevice->mapMemory(m_buffers[BUFFER_SCENE_LIGHT]);
    }

    // create transform buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_meshNodeList.size() * sizeof(glm::mat4)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_TRANSFORM], m_transformInfos.data(), true);
    }

    // create index buffer
    {
        auto &indicesList = m_scene->m_indices;
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(indicesList.size()),
            .usage = BUFFER_USAGE_INDEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_INDEX], indicesList.data());
    }

    // create vertex buffer
    {
        auto &verticesList = m_scene->m_vertices;
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(verticesList.size()),
            .usage = BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_VERTEX], verticesList.data());
    }

    // create material buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_scene->m_materials.size() * sizeof(Material)),
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_MATERIAL], m_scene->m_materials.data());
    }

    // load scene image to gpu
    for(const auto &image : m_scene->m_images)
    {
        ImageCreateInfo createInfo{
            .extent = { image->width, image->height, 1 },
            .mipLevels = calculateFullMipLevels(image->width, image->height),
            .usage = IMAGE_USAGE_SAMPLED_BIT,
            .format = FORMAT_R8G8B8A8_UNORM,
            .tiling = IMAGE_TILING_OPTIMAL,
        };

        VulkanImage *texture{};
        m_pDevice->createDeviceLocalImage(createInfo, &texture, image->data);
        m_images[IMAGE_SCENE_TEXTURES].push_back(texture);
    }

}

void VulkanSceneRenderer::_initSkybox()
{
    // create skybox cubemap
    {
        uint32_t cubeMapWidth {}, cubeMapHeight {};
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        uint32_t mipLevels = 0;

        std::array<VulkanBuffer*, 6> stagingBuffers;
        for (auto idx = 0; idx < 6; idx++)
        {
            auto image = m_scene->m_images[0];
            cubeMapWidth = image->width;
            cubeMapHeight = image->height;

            {
                BufferCreateInfo createInfo{
                    .size = static_cast<uint32_t>(image->data.size()),
                    .usage = BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };

                m_pDevice->createBuffer(createInfo, &stagingBuffers[idx]);
                m_pDevice->mapMemory(stagingBuffers[idx]);
                stagingBuffers[idx]->copyTo(image->data.data());
                m_pDevice->unMapMemory(stagingBuffers[idx]);
            }
        }
        mipLevels = calculateFullMipLevels(cubeMapWidth, cubeMapHeight);

        std::vector<VkBufferImageCopy> bufferCopyRegions;
        for (uint32_t face = 0; face < 6; face++)
        {
            for (uint32_t level = 0; level < mipLevels; level++)
            {
                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = level;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = cubeMapWidth >> level;
                bufferCopyRegion.imageExtent.height = cubeMapHeight >> level;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = 0;
                bufferCopyRegions.push_back(bufferCopyRegion);
            }
        }

        // Image barrier for optimal image (target)
        // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
        VkImageSubresourceRange subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .layerCount = 6,
        };

        VulkanImage * cubeMap {};
        ImageCreateInfo imageCI{
            .extent = {cubeMapWidth, cubeMapHeight, 1},
            .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            .imageType = IMAGE_TYPE_2D,
            .mipLevels = mipLevels,
            .arrayLayers = 6,
            .usage = IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_DST_BIT,
            .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .format = FORMAT_R8G8B8A8_UNORM,
        };
        m_pDevice->createImage(imageCI, &cubeMap);

        m_pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer* pCommandBuffer){
            pCommandBuffer->transitionImageLayout(cubeMap, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                  &subresourceRange);
            // Copy the cube map faces from the staging buffer to the optimal tiled image
            for(uint32_t idx = 0; idx < 6; idx++)
            {
                pCommandBuffer->copyBufferToImage(stagingBuffers[idx], cubeMap, { bufferCopyRegions[idx] });
            }
            pCommandBuffer->transitionImageLayout(cubeMap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                  &subresourceRange);
        });

        // Create image view
        VkImageViewCreateInfo view = aph::init::imageViewCreateInfo();
        // Cube map view type
        view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        view.format = imageFormat;
        view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        view.subresourceRange.layerCount = 6;
        view.subresourceRange.levelCount = mipLevels;
        view.image = cubeMap->getHandle();
        VK_CHECK_RESULT(vkCreateImageView(m_pDevice->getHandle(), &view, nullptr, &m_cubeMapView));
    }
}
}  // namespace aph
