#include "sceneRenderer.h"

#include "common/assetManager.h"

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"

#include "api/vulkan/device.h"

namespace aph
{
VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer) :
    m_pDevice(renderer->getDevice()),
    m_pRenderer(renderer)
{
}

void VulkanSceneRenderer::loadResources()
{
    _loadScene();
    _initSetLayout();
    _initSampler();
    _initForward();
    _initPostFx();
    _initRenderData();
}

void VulkanSceneRenderer::cleanupResources()
{
    for (auto *pipeline : m_pipelines){
        m_pDevice->destroyPipeline(pipeline);
    }

    for(auto *setLayout : m_setLayouts)
    {
        m_pDevice->destroyDescriptorSetLayout(setLayout);
    }

    for(auto &texture : m_textures)
    {
        m_pDevice->destroyImage(texture);
    }

    for(uint32_t idx = 0; idx < m_pRenderer->getSwapChain()->getImageCount(); idx++)
    {
        m_pDevice->destroyImage(m_forward.colorImages[idx]);
        m_pDevice->destroyImage(m_forward.depthImages[idx]);
    }

    for (auto &buffer : m_buffers)
    {
        m_pDevice->destroyBuffer(buffer);
    }

    for(auto &sampler : m_samplers)
    {
        vkDestroySampler(m_pDevice->getHandle(), sampler, nullptr);
    }

    for(auto &ubData : m_uniformDataList)
    {
        m_pDevice->destroyBuffer(ubData->m_buffer);
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
    commandBuffer->setViewport(&viewport);
    commandBuffer->setSissor(&scissor);

    // forward pass
    {
        VulkanImageView *pColorAttachment = m_forward.colorImages[imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color {{0.1f,0.1f,0.1f,1.0f}}},
        };

        VulkanImageView *pDepthAttachment = m_forward.depthImages[imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pDepthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {.depthStencil {1.0f, 0}},
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
        commandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 2, 1, &m_samplerSet);

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        commandBuffer->transitionImageLayout(pDepthAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        commandBuffer->beginRendering(renderingInfo);
        for(auto &renderData : m_renderDataList)
        {
            _drawRenderData(renderData, m_pipelines[PIPELINE_GRAPHICS_FORWARD], commandBuffer);
        }
        commandBuffer->endRendering();

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                             VK_IMAGE_LAYOUT_GENERAL);
    }

    {
        VulkanImageView *pColorAttachment = m_pRenderer->getSwapChain()->getImage(imageIdx)->getImageView();

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL);
        commandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);

        {
            std::vector<VkWriteDescriptorSet> writes{
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0,
                                            &m_forward.colorImages[imageIdx]->getImageView()->getDescInfoMap(
                                                VK_IMAGE_LAYOUT_GENERAL)),

                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
                                            &m_pRenderer->getSwapChain()->getImage(imageIdx)->getImageView()->getDescInfoMap(
                                                VK_IMAGE_LAYOUT_GENERAL)),
            };

            vkCmdPushDescriptorSetKHR(commandBuffer->getHandle(),
                                      m_pipelines[PIPELINE_COMPUTE_POSTFX]->getBindPoint(),
                                      m_pipelines[PIPELINE_COMPUTE_POSTFX]->getPipelineLayout(),
                                      0, writes.size(), writes.data());
        }

        // commandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_COMPUTE_POSTFX], 1, 1, &m_samplerSet);
        commandBuffer->dispatch(pColorAttachment->getImage()->getWidth(),
                                pColorAttachment->getImage()->getHeight(),
                                1);
        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_GENERAL,
                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    commandBuffer->end();
}

void VulkanSceneRenderer::update(float deltaTime)
{
    for(auto &ubo : m_uniformDataList)
    {
        ubo->update();
    }
}

void VulkanSceneRenderer::_initRenderData()
{
    {
        SceneInfo info{
            .ambient = glm::vec4(m_scene->getAmbient(), 0.0f),
            .cameraCount = static_cast<uint32_t>(m_cameraInfos.size()),
            .lightCount = static_cast<uint32_t>(m_lightInfos.size()),
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

        std::vector<VkDescriptorImageInfo> m_textureInfos{};
        for(auto &texture : m_textures)
        {
            VkDescriptorImageInfo info{
                .imageView = texture->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            m_textureInfos.push_back(info);
        }

        std::vector<VkWriteDescriptorSet> writes{
            sceneInfoSetWrite,
            aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_cameraInfos.data(),
                                          m_cameraInfos.size()),
            aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, m_lightInfos.data(),
                                          m_lightInfos.size()),
            aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3, m_textureInfos.data(),
                                          m_textureInfos.size()),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    // write material descriptor set
    for(const auto &material : m_scene->m_materials)
    {
        auto *set = m_setLayouts[SET_LAYOUT_MATERIAL]->allocateSet();
        std::vector<VkWriteDescriptorSet> descriptorWrites{};

        {
            VkWriteDescriptorSetInlineUniformBlockEXT writeDescriptorSetInlineUniformBlock{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT,
                .dataSize = sizeof(Material),
                .pData = &material,
            };

            VkWriteDescriptorSet writeDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = &writeDescriptorSetInlineUniformBlock,
                .dstSet = set,
                .dstBinding = 0,
                .descriptorCount = sizeof(Material),
                .descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
            };

            descriptorWrites.push_back(writeDescriptorSet);
        }

        vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()),
                                descriptorWrites.data(), 0, nullptr);
        m_materialSets.push_back(set);
    }
}

void VulkanSceneRenderer::_loadScene()
{
    // load scene image to gpu
    for(auto &image : m_scene->getImages())
    {

        ImageCreateInfo ci{
            .extent = {image->width, image->height, 1},
            .mipLevels = calculateFullMipLevels(image->width, image->height),
            .usage = IMAGE_USAGE_SAMPLED_BIT,
            .format = FORMAT_R8G8B8A8_UNORM,
            .tiling = IMAGE_TILING_OPTIMAL,
        };

        VulkanImage* texture {};
        m_pDevice->createDeviceLocalImage(ci, &texture, image->data);
        m_textures.push_back(texture);
    }

    {
        auto &indicesList = m_scene->m_indices;
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(indicesList.size()),
            .alignment = 0,
            .usage = BUFFER_USAGE_INDEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_INDEX], indicesList.data());
    }

    {
        auto &verticesList = m_scene->m_vertices;
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(verticesList.size()),
            .alignment = 0,
            .usage = BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_VERTEX], verticesList.data());
    }

    std::queue<std::shared_ptr<SceneNode>> q;
    q.push(m_scene->getRootNode());

    while(!q.empty())
    {
        auto node = q.front();
        q.pop();

        switch(node->m_attachType)
        {
        case ObjectType::MESH:
        {
            auto renderData = std::make_shared<VulkanRenderData>(node);
            m_renderDataList.push_back(renderData);
        }
        break;
        case ObjectType::CAMERA:
        {
            auto ubo = std::make_shared<VulkanUniformData>(node);
            {
                auto object = node->getObject<Camera>();
                object->load();
                ubo->m_object = object;
                BufferCreateInfo createInfo{
                    .size = object->getDataSize(),
                    .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };
                VK_CHECK_RESULT(m_pDevice->createBuffer(createInfo, &ubo->m_buffer, object->getData()));
                ubo->m_buffer->map();
            }
            m_cameraInfos.push_back({ ubo->m_buffer->getHandle(), 0, VK_WHOLE_SIZE });
            m_uniformDataList.push_front(std::move(ubo));
        }
        break;
        case ObjectType::LIGHT:
        {
            auto ubo = std::make_shared<VulkanUniformData>(node);
            {
                auto object = node->getObject<Light>();
                object->load();
                ubo->m_object = object;
                BufferCreateInfo createInfo{
                    .size = object->getDataSize(),
                    .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };
                VK_CHECK_RESULT(m_pDevice->createBuffer(createInfo, &ubo->m_buffer, object->getData()));
                ubo->m_buffer->map();
            }
            m_lightInfos.push_back({ ubo->m_buffer->getHandle(), 0, VK_WHOLE_SIZE });
            m_uniformDataList.push_back(std::move(ubo));
        }
        break;
        default:
            assert("unattached scene node.");
            break;
        }

        for(const auto &subNode : node->children)
        {
            q.push(subNode);
        }
    }
}

void VulkanSceneRenderer::_initPostFx()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_POSTFX] };
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_COMPUTE_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "postFX.comp.spv") },
        };
        VK_CHECK_RESULT(m_pDevice->createComputePipeline(pipelineCreateInfo, &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
    }
}

void VulkanSceneRenderer::_initForward()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    VkExtent2D imageExtent = m_pRenderer->getSwapChain()->getExtent();

    m_forward.colorImages.resize(imageCount);
    m_forward.depthImages.resize(imageCount);

    // frame buffer
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto &colorImage = m_forward.colorImages[idx];
        auto &depthImage = m_forward.depthImages[idx];

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

    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = { m_pRenderer->getSwapChain()->getSurfaceFormat() };
        pipelineCreateInfo.renderingCreateInfo = VkPipelineRenderingCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_SCENE],
                                          m_setLayouts[SET_LAYOUT_MATERIAL],
                                          m_setLayouts[SET_LAYOUT_SAMP] };
        pipelineCreateInfo.constants.push_back(
            aph::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv") },
        };

        VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_FORWARD]));
    }

    {
        m_sceneSet = m_setLayouts[SET_LAYOUT_SCENE]->allocateSet();
    }
}

void VulkanSceneRenderer::_drawRenderData(const std::shared_ptr<VulkanRenderData> &renderData, VulkanPipeline *pipeline,
                                          VulkanCommandBuffer *drawCmd)
{
    auto mesh = renderData->m_node->getObject<Mesh>();
    {
        auto matrix = renderData->m_node->matrix;
        auto currentNode = renderData->m_node->parent;
        while(currentNode)
        {
            matrix = currentNode->matrix * matrix;
            currentNode = currentNode->parent;
        }
        drawCmd->pushConstants(pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                               &matrix);
    }
    drawCmd->bindVertexBuffers(mesh->m_vertexOffset, 1, m_buffers[BUFFER_SCENE_VERTEX], { 0 });
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
        }
        drawCmd->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX], mesh->m_indexOffset, indexType);
    }
    for(const auto &subset : mesh->m_subsets)
    {
        if(subset.indexCount > 0)
        {
            auto &materialSet = m_materialSets[subset.materialIndex];
            drawCmd->bindDescriptorSet(pipeline, 1, 1, &materialSet);
            if(subset.hasIndices)
            {
                drawCmd->drawIndexed(subset.indexCount, 1, subset.firstIndex, 0, 0);
            }
            else
            {
                drawCmd->draw(subset.vertexCount, 1, subset.firstVertex, 0);
            }
        }
    }
}

void VulkanSceneRenderer::_initSampler()
{
    // texture
    {
        VkSamplerCreateInfo samplerInfo = aph::init::samplerCreateInfo();
        samplerInfo.maxLod = calculateFullMipLevels(2048, 2048);
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &samplerInfo, nullptr, &m_samplers[SAMP_TEXTURE]));
    }

    // shadow
    {
        // VkSamplerCreateInfo createInfo = aph::init::samplerCreateInfo();
        // createInfo.magFilter = m_shadowPass.filter;
        // createInfo.minFilter = m_shadowPass.filter;
        // VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &m_sampler.shadow));
    }

    // allocate set and update
    {
        auto &set = m_samplerSet;
        set = m_setLayouts[SET_LAYOUT_SAMP]->allocateSet();

        std::vector<VkDescriptorImageInfo> imageInfos{
            {
                .sampler = m_samplers[SAMP_TEXTURE],
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            },
        };
        std::vector<VkWriteDescriptorSet> writes{
            aph::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLER, 0, imageInfos.data(), imageInfos.size()),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
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
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1,
                                                  m_cameraInfos.size()),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2,
                                                  m_lightInfos.size()),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 3,
                                                  m_textures.size()),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_SCENE]);
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

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK, VK_SHADER_STAGE_FRAGMENT_BIT,
                                                  0,
                                                  sizeof(Material)),  // material info
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_MATERIAL]);
    }

    // sampler
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_SAMP]);
    }
}
}  // namespace aph
