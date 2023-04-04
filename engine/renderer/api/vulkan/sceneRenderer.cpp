#include "sceneRenderer.h"

#include "common/assetManager.h"

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"

#include "api/vulkan/device.h"

namespace aph
{

namespace
{

VulkanBuffer *createBuffer(VulkanDevice *pDevice,
                           VulkanQueue *pQueue,
                           const void *data,
                           VkDeviceSize size,
                           VkBufferUsageFlags usage)
{
    VulkanBuffer *buffer = nullptr;
    // setup vertex buffer
    {
        VkDeviceSize bufferSize = size;
        // using staging buffer
        aph::VulkanBuffer *stagingBuffer;
        {
            BufferCreateInfo createInfo{
                .size = static_cast<uint32_t>(bufferSize),
                .usage = BUFFER_USAGE_TRANSFER_SRC_BIT,
                .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
            };
            pDevice->createBuffer(createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(data, static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{
                .size = static_cast<uint32_t>(bufferSize),
                .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            };
            pDevice->createBuffer(createInfo, &buffer);
        }

        pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *cmd){
            cmd->copyBuffer(stagingBuffer, buffer, bufferSize);
        });

        pDevice->destroyBuffer(stagingBuffer);
    }
    return buffer;
}
VulkanImage* createTexture(VulkanDevice *pDevice, VulkanQueue *pQueue, uint32_t width, uint32_t height, void *data,
                         uint32_t dataSize, bool genMipmap = false)
{
    uint32_t texMipLevels = genMipmap ? calculateFullMipLevels(width, height) : 1;

    // Load texture from image buffer
    aph::VulkanBuffer *stagingBuffer;
    {
        BufferCreateInfo createInfo{};
        createInfo.size = dataSize;
        createInfo.usage = BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
        pDevice->createBuffer(createInfo, &stagingBuffer);

        stagingBuffer->map();
        stagingBuffer->copyTo(data, dataSize);
        stagingBuffer->unmap();
    }

    VulkanImage* texture{};

    {
        ImageCreateInfo createInfo{
            .extent = { width, height, 1 },
            .mipLevels = texMipLevels,
            .usage = IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT,
            .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .format = FORMAT_R8G8B8A8_UNORM,
            .tiling = IMAGE_TILING_OPTIMAL,
        };

        pDevice->createImage(createInfo, &texture);

        pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *cmd){
            cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            cmd->copyBufferToImage(stagingBuffer, texture);
            cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        });

        pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *cmd){
            // generate mipmap chains
            for(int32_t i = 1; i < texMipLevels; i++)
            {
                VkImageBlit imageBlit{};

                // Source
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
                imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
                imageBlit.srcOffsets[1].z = 1;

                // Destination
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstOffsets[1].x = int32_t(width >> i);
                imageBlit.dstOffsets[1].y = int32_t(height >> i);
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange mipSubRange = {};
                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = 1;

                // Prepare current mip level as image blit destination
                cmd->imageMemoryBarrier(texture, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange);

                // Blit from previous level
                cmd->blitImage(texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                // Prepare current mip level as image blit source for next level
                cmd->imageMemoryBarrier(texture, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange);
            }

            cmd->transitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

    }

    pDevice->destroyBuffer(stagingBuffer);

    return texture;
}

}  // namespace

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
    if(m_forwardPass.pipeline != nullptr)
    {
        m_pDevice->destroyPipeline(m_forwardPass.pipeline);
    }

    if(m_postFxPass.pipeline != nullptr)
    {
        m_pDevice->destroyPipeline(m_postFxPass.pipeline);
    }

    {
        m_pDevice->destroyDescriptorSetLayout(m_setLayout.pMaterial);
        m_pDevice->destroyDescriptorSetLayout(m_setLayout.pObject);
        m_pDevice->destroyDescriptorSetLayout(m_setLayout.pSampler);
        m_pDevice->destroyDescriptorSetLayout(m_setLayout.pScene);
        m_pDevice->destroyDescriptorSetLayout(m_setLayout.pOffScreen);
    }

    for(auto &texture : m_textures)
    {
        m_pDevice->destroyImage(texture);
    }

    for(uint32_t idx = 0; idx < m_pRenderer->getSwapChain()->getImageCount(); idx++)
    {
        m_pDevice->destroyImage(m_forwardPass.colorAttachments[idx]);
        m_pDevice->destroyImage(m_forwardPass.depthAttachments[idx]);
    }
    vkDestroySampler(m_pDevice->getHandle(), m_sampler.postFX, nullptr);
    vkDestroySampler(m_pDevice->getHandle(), m_sampler.texture, nullptr);

    for(auto &renderData : m_renderDataList)
    {
        m_pDevice->destroyBuffer(renderData->m_vertexBuffer);
        m_pDevice->destroyBuffer(renderData->m_indexBuffer);
        m_pDevice->destroyBuffer(renderData->m_objectUB);
    }

    for(auto &ubData : m_uniformDataList)
    {
        m_pDevice->destroyBuffer(ubData->m_buffer);
    }

    m_pDevice->destroyBuffer(m_postFxPass.quadVB);
    m_pDevice->destroyBuffer(m_sceneInfoUB);
}

void VulkanSceneRenderer::recordDrawSceneCommands()
{
    VkExtent2D extent{
        .width = m_pRenderer->getWindowWidth(),
        .height = m_pRenderer->getWindowHeight(),
    };
    VkViewport viewport = aph::init::viewport(extent);
    VkRect2D scissor = aph::init::rect2D(extent);

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkCommandBufferBeginInfo beginInfo = aph::init::commandBufferBeginInfo();

    uint32_t commandIndex = m_pRenderer->getCurrentFrameIndex();
    VulkanCommandBuffer *commandBuffer = m_pRenderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->setViewport(&viewport);
    commandBuffer->setSissor(&scissor);

    // forward pass
    {
        VulkanImageView * pColorAttachment = m_forwardPass.colorAttachments[m_pRenderer->getCurrentImageIndex()]->getImageView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clearValues[0],
        };

        VulkanImageView * pDepthAttachment = m_forwardPass.depthAttachments[m_pRenderer->getCurrentImageIndex()]->getImageView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pDepthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = clearValues[1],
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &forwardColorAttachmentInfo,
            .pDepthAttachment = &forwardDepthAttachmentInfo,
        };

        renderingInfo.renderArea = {
            .offset = {0, 0},
            .extent = extent,
        };

        commandBuffer->bindPipeline(m_forwardPass.pipeline);
        commandBuffer->bindDescriptorSet(m_forwardPass.pipeline, PASS_FORWARD::SET_SCENE, 1, &m_sceneSets[commandIndex]);
        commandBuffer->bindDescriptorSet(m_forwardPass.pipeline, PASS_FORWARD::SET_SAMPLER, 1, &m_sampler.set);
        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        commandBuffer->transitionImageLayout(pDepthAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        commandBuffer->beginRendering(renderingInfo);
        for(auto &renderable : m_renderDataList)
        {
            _drawRenderData(renderable, m_forwardPass.pipeline, commandBuffer);
        }
        commandBuffer->endRendering();
        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    {
        VulkanImageView* pColorAttachment = m_postFxPass.colorAttachments[m_pRenderer->getCurrentImageIndex()]->getImageView();
        VkRenderingAttachmentInfo postFxColorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clearValues[0],
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &postFxColorAttachmentInfo,
            .pDepthAttachment = nullptr,
        };
        renderingInfo.renderArea = {
            .offset = {0, 0},
            .extent = extent,
        };

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        commandBuffer->beginRendering(renderingInfo);
        commandBuffer->bindVertexBuffers(0, 1, m_postFxPass.quadVB, {0});
        commandBuffer->bindPipeline(m_postFxPass.pipeline);
        commandBuffer->bindDescriptorSet(m_postFxPass.pipeline, PASS_POSTFX::SET_OFFSCREEN, 1,
                                            &m_postFxPass.sets[m_pRenderer->getCurrentImageIndex()]);
        commandBuffer->bindDescriptorSet(m_postFxPass.pipeline, PASS_POSTFX::SET_SAMPLER, 1, &m_sampler.set);
        commandBuffer->draw(6, 1, 0, 0);
        commandBuffer->endRendering();
        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    commandBuffer->end();
}

void VulkanSceneRenderer::update(float deltaTime)
{
    for(auto &ubo : m_uniformDataList)
    {
        ubo->m_buffer->copyTo(ubo->m_object->getData());
    }
}

void VulkanSceneRenderer::_initRenderData()
{
    // [TODO] object data
    {
        BufferCreateInfo bufferCI{
            .size = sizeof(SceneInfo),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_COHERENT_BIT | MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(m_pDevice->createBuffer(bufferCI, &m_sceneInfoUB, &m_sceneInfo));
        m_sceneInfoUB->setupDescriptor();
    }

    for(auto &set : m_sceneSets)
    {
        std::vector<VkWriteDescriptorSet> writes{
            aph::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &m_sceneInfoUB->getBufferInfo()),
            aph::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_cameraInfos.data(),
                                          m_cameraInfos.size()),
            aph::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, m_lightInfos.data(),
                                          m_lightInfos.size()),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    for(auto &renderData : m_renderDataList)
    {
        // object info
        {
            ObjectInfo objInfo{};
            BufferCreateInfo bufferCI{
                .size = sizeof(ObjectInfo),
                .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
            };
            VK_CHECK_RESULT(m_pDevice->createBuffer(bufferCI, &renderData->m_objectUB, &objInfo));
            renderData->m_objectUB->setupDescriptor();

            renderData->m_objectSet = m_setLayout.pObject->allocateSet();

            std::vector<VkWriteDescriptorSet> descriptorWrites{
                aph::init::writeDescriptorSet(
                renderData->m_objectSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                &renderData->m_objectUB->getBufferInfo()) };

            vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
        }

        auto writeDescInfo = [](VulkanImage *pImage, VkDescriptorSet set, uint32_t binding) -> VkWriteDescriptorSet {
            return aph::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, binding,
                                                 &pImage->getImageView()->getDescInfoMap(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        };

        for(auto &material : m_scene->getMaterials())
        {
            // write descriptor set
            auto set = m_setLayout.pMaterial->allocateSet();
            {
                std::vector<VkWriteDescriptorSet> descriptorWrites{};

                {
                    // BufferCreateInfo bufferCI{
                    //     .size = sizeof(Material),
                    //     .alignment = 0,
                    //     .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    //     .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    // };
                    // m_pDevice->createBuffer(bufferCI, &matInfoUB, &material);
                    // matInfoUB->setupDescriptor();

                    // create material buffer Info
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

                auto bindingBits = MATERIAL_BINDING_PBR;
                if(bindingBits & MATERIAL_BINDING_BASECOLOR)
                {
                    std::cerr << "material id: [" << material->id << "] [base color]: ";
                    if(material->baseColorTextureIndex > -1)
                    {
                        descriptorWrites.push_back(writeDescInfo(m_textures[material->baseColorTextureIndex], set, 1));
                        std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                    }
                    else
                    {
                        std::cerr << "texture not found." << std::endl;
                    }
                }
                if(bindingBits & MATERIAL_BINDING_NORMAL)
                {
                    std::cerr << "material id: [" << material->id << "] [normal]: ";
                    if(material->normalTextureIndex > -1)
                    {
                        descriptorWrites.push_back(writeDescInfo(m_textures[material->normalTextureIndex], set, 2));
                        std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                    }
                    else
                    {
                        std::cerr << "texture not found." << std::endl;
                    }
                }
                if(bindingBits & MATERIAL_BINDING_PHYSICAL)
                {
                    std::cerr << "material id: [" << material->id << "] [physical desc]: ";
                    if(material->metallicFactor > -1)
                    {
                        descriptorWrites.push_back(writeDescInfo(m_textures[material->metallicRoughnessTextureIndex], set, 3));
                        std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                    }
                    else
                    {
                        std::cerr << "texture not found." << std::endl;
                    }
                }
                if(bindingBits & MATERIAL_BINDING_AO)
                {
                    std::cerr << "material id: [" << material->id << "] [ao]: ";
                    if(material->occlusionTextureIndex > -1)
                    {
                        descriptorWrites.push_back(writeDescInfo(m_textures[material->occlusionTextureIndex], set, 4));
                        std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                    }
                    else
                    {
                        std::cerr << "texture not found." << std::endl;
                    }
                }
                if(bindingBits & MATERIAL_BINDING_EMISSIVE)
                {
                    std::cerr << "material id: [" << material->id << "] [emissive]: ";
                    if(material->emissiveTextureIndex > -1)
                    {
                        descriptorWrites.push_back(writeDescInfo(m_textures[material->emissiveTextureIndex], set, 5));
                        std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                    }
                    else
                    {
                        std::cerr << "texture not found." << std::endl;
                    }
                }

                vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()),
                                       descriptorWrites.data(), 0, nullptr);
            }

            m_materialSetMaps[material] = set;
        }
    }
}

void VulkanSceneRenderer::_loadScene()
{
    for(auto &image : m_scene->getImages())
    {
        // raw image data
        uint8_t *imageData = image->data.data();
        uint32_t imageDataSize = image->data.size();
        uint32_t width = image->width;
        uint32_t height = image->height;

        auto texture =
            createTexture(m_pDevice, m_pRenderer->getGraphicsQueue(), width, height, imageData, imageDataSize, true);
        m_textures.push_back(texture);
    }

    m_sceneInfo.ambient = glm::vec4(m_scene->getAmbient(), 1.0f);
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
            {
                auto mesh = node->getObject<Mesh>();
                auto &vertices = mesh->m_vertices;
                auto &indices = mesh->m_indices;
                // load buffer
                assert(!vertices.empty());
                renderData->m_vertexBuffer =
                    createBuffer(m_pDevice, m_pRenderer->getGraphicsQueue(), vertices.data(),
                                 sizeof(vertices[0]) * vertices.size(), BUFFER_USAGE_VERTEX_BUFFER_BIT);
                if(!indices.empty())
                {
                    renderData->m_indexBuffer =
                        createBuffer(m_pDevice, m_pRenderer->getGraphicsQueue(), indices.data(),
                                     sizeof(indices[0]) * indices.size(), BUFFER_USAGE_INDEX_BUFFER_BIT);
                }
            }
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
                ubo->m_buffer->setupDescriptor();
                ubo->m_buffer->map();
            }
            m_cameraInfos.push_back(ubo->m_buffer->getBufferInfo());
            m_uniformDataList.push_front(std::move(ubo));
            m_sceneInfo.cameraCount++;
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
                ubo->m_buffer->setupDescriptor();
                ubo->m_buffer->map();
            }
            m_lightInfos.push_back(ubo->m_buffer->getBufferInfo());
            m_uniformDataList.push_back(std::move(ubo));
            m_sceneInfo.lightCount++;
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

void VulkanSceneRenderer::_initSkyboxResource()
{
}

void VulkanSceneRenderer::_initPostFx()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    m_postFxPass.colorAttachments.resize(imageCount);
    m_postFxPass.sets.resize(imageCount);

    // buffer
    {
        float quadVertices[] = { // positions   // texCoords
                                 -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

                                 -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f
        };

        BufferCreateInfo bufferCI{
            .size = sizeof(quadVertices),
            .alignment = 0,
            .usage = BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        VK_CHECK_RESULT(m_pDevice->createBuffer(bufferCI, &m_postFxPass.quadVB, quadVertices));
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.setLayouts = { m_setLayout.pOffScreen, m_setLayout.pSampler };
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "postFX.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "postFX.frag.spv") },
        };
        std::vector<VkVertexInputBindingDescription> bindingDescs{
            { 0, 4 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX },
        };
        std::vector<VkVertexInputAttributeDescription> attrDescs{
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 }, { 1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float) }
        };
        pipelineCreateInfo.vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size()),
            .pVertexBindingDescriptions = bindingDescs.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size()),
            .pVertexAttributeDescriptions = attrDescs.data(),
        };

        auto swapchainFormat = m_pRenderer->getSwapChain()->getSurfaceFormat();
        pipelineCreateInfo.renderingCreateInfo = VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchainFormat,
        };
        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_postFxPass.pipeline));
    }

    // color attachment
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto & colorAttachment = m_postFxPass.colorAttachments[idx];
        colorAttachment = m_pRenderer->getSwapChain()->getImage(idx);

        auto &set = m_postFxPass.sets[idx];
        set = m_setLayout.pOffScreen->allocateSet();

        std::vector<VkWriteDescriptorSet> writes{
            aph::init::writeDescriptorSet(
                set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0,
                &m_forwardPass.colorAttachments[idx]->getImageView()->getDescInfoMap(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
        };

        vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void VulkanSceneRenderer::_initForward()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    VkExtent2D imageExtent = m_pRenderer->getSwapChain()->getExtent();

    m_forwardPass.colorAttachments.resize(imageCount);
    m_forwardPass.depthAttachments.resize(imageCount);

    // frame buffer
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto &colorImage = m_forwardPass.colorAttachments[idx];
        auto &depthImage = m_forwardPass.depthAttachments[idx];

        {
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .imageType = IMAGE_TYPE_2D,
                .usage = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_SAMPLED_BIT,
                .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImage(createInfo, &colorImage);
        }

        {
            VkFormat depthFormat = m_pDevice->getDepthFormat();
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = static_cast<Format>(depthFormat),
                .tiling = IMAGE_TILING_OPTIMAL,
            };
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depthImage));
        }

        m_pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *cmd){
            cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        });
    }

    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = {m_pRenderer->getSwapChain()->getSurfaceFormat()};
        pipelineCreateInfo.renderingCreateInfo = VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.setLayouts = { m_setLayout.pScene, m_setLayout.pObject, m_setLayout.pMaterial,
                                          m_setLayout.pSampler };
        pipelineCreateInfo.constants.push_back(
            aph::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv") },
        };

        VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_forwardPass.pipeline));
    }

    m_sceneSets.resize(m_pRenderer->getCommandBufferCount());
    for(auto &set : m_sceneSets)
    {
        set = m_setLayout.pScene->allocateSet();
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
    drawCmd->bindVertexBuffers(0, 1, renderData->m_vertexBuffer, {0});
    if(renderData->m_indexBuffer)
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
        drawCmd->bindIndexBuffers(renderData->m_indexBuffer, 0, indexType);
    }
    for(const auto &subset : mesh->m_subsets)
    {
        if(subset.indexCount > 0)
        {
            auto &material = m_scene->getMaterials()[subset.materialIndex];
            auto &materialSet = m_materialSetMaps[material];
            drawCmd->bindDescriptorSet(pipeline, 2, 1, &materialSet);
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

        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &samplerInfo, nullptr, &m_sampler.texture));
    }

    // shadow
    {
        // VkSamplerCreateInfo createInfo = aph::init::samplerCreateInfo();
        // createInfo.magFilter = m_shadowPass.filter;
        // createInfo.minFilter = m_shadowPass.filter;
        // VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &m_sampler.shadow));
    }

    // postFX
    {
        VkSamplerCreateInfo createInfo = aph::init::samplerCreateInfo();
        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &m_sampler.postFX));
    }

    // allocate set
    {
        auto &set = m_sampler.set;
        set = m_setLayout.pSampler->allocateSet();

        std::vector<VkDescriptorImageInfo> imageInfos{
            {
                .sampler = m_sampler.texture,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            },
            {
                .sampler = m_sampler.postFX,
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
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1,
                                                  m_sceneInfo.cameraCount),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2,
                                                  m_sceneInfo.lightCount),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pScene);
    }

    // off screen texture
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pOffScreen);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                  0, sizeof(Material)),  // material info
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pMaterial);
    }

    // object
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pObject);
    }

    // sampler
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pSampler);
    }
}
}  // namespace aph
