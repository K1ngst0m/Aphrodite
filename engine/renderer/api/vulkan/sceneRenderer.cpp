#include "sceneRenderer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "common/assetManager.h"
#include "descriptorSetLayout.h"
#include "device.h"
#include "framebuffer.h"
#include "imageView.h"
#include "pipeline.h"
#include "renderData.h"
#include "renderer/api/vulkan/uiRenderer.h"
#include "renderpass.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"
#include "swapChain.h"
#include "vkInit.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkanRenderer.h"

namespace vkl
{

namespace
{
GpuTexture createTexture(VulkanDevice *pDevice, uint32_t width, uint32_t height, void *data, uint32_t dataSize,
                         bool genMipmap = false)
{
    uint32_t texMipLevels = genMipmap ? calculateFullMipLevels(width, height) : 1;

    // Load texture from image buffer
    vkl::VulkanBuffer *stagingBuffer;
    {
        BufferCreateInfo createInfo{};
        createInfo.size = dataSize;
        createInfo.usage = BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
        pDevice->createBuffer(&createInfo, &stagingBuffer);

        stagingBuffer->map();
        stagingBuffer->copyTo(data, dataSize);
        stagingBuffer->unmap();
    }

    GpuTexture texture{};

    {
        ImageCreateInfo createInfo{};
        createInfo.extent = { width, height, 1 };
        createInfo.format = FORMAT_R8G8B8A8_UNORM;
        createInfo.tiling = IMAGE_TILING_OPTIMAL;
        createInfo.usage = IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT;
        createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        createInfo.mipLevels = texMipLevels;

        pDevice->createImage(&createInfo, &texture.image);

        auto *cmd = pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cmd->cmdCopyBufferToImage(stagingBuffer, texture.image);
        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        pDevice->endSingleTimeCommands(cmd);

        cmd = pDevice->beginSingleTimeCommands(VK_QUEUE_GRAPHICS_BIT);

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
            cmd->cmdImageMemoryBarrier(texture.image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange);

            // Blit from previous level
            cmd->cmdBlitImage(texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.image,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

            // Prepare current mip level as image blit source for next level
            cmd->cmdImageMemoryBarrier(texture.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange);
        }

        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        pDevice->endSingleTimeCommands(cmd);
    }

    {
        ImageViewCreateInfo createInfo{};
        createInfo.format = FORMAT_R8G8B8A8_UNORM;
        createInfo.viewType = IMAGE_VIEW_TYPE_2D;
        createInfo.subresourceRange.levelCount = texMipLevels;
        pDevice->createImageView(&createInfo, &texture.imageView, texture.image);
    }

    pDevice->destroyBuffer(stagingBuffer);

    return texture;
}
}  // namespace

std::unique_ptr<VulkanSceneRenderer> VulkanSceneRenderer::Create(const std::shared_ptr<VulkanRenderer> &renderer)
{
    auto instance = std::make_unique<VulkanSceneRenderer>(renderer);
    return instance;
}

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

    for(uint32_t idx = 0; idx < m_pRenderer->getSwapChain()->getImageCount(); idx++)
    {
        m_pDevice->destroyFramebuffers(m_postFxPass.framebuffers[idx]);
        m_pDevice->destroyFramebuffers(m_forwardPass.framebuffers[idx]);
        m_pDevice->destroyImage(m_forwardPass.colorImages[idx]);
        m_pDevice->destroyImageView(m_postFxPass.colorImageViews[idx]);
        m_pDevice->destroyImageView(m_forwardPass.colorImageViews[idx]);
        m_pDevice->destroyImage(m_forwardPass.depthImages[idx]);
        m_pDevice->destroyImageView(m_forwardPass.depthImageViews[idx]);
    }
    vkDestroySampler(m_pDevice->getHandle(), m_sampler.postFX, nullptr);
    vkDestroySampler(m_pDevice->getHandle(), m_sampler.texture, nullptr);
    // vkDestroySampler(m_pDevice->getHandle(), m_samplers.cubeMap, nullptr);
    // vkDestroySampler(m_pDevice->getHandle(), m_samplers.shadow, nullptr);

    m_pDevice->destroyBuffer(m_postFxPass.quadVB);
    m_pDevice->destroyBuffer(m_sceneInfoUB);
    vkDestroyRenderPass(m_pDevice->getHandle(), m_forwardPass.renderPass->getHandle(), nullptr);
    vkDestroyRenderPass(m_pDevice->getHandle(), m_postFxPass.renderPass->getHandle(), nullptr);
}

void VulkanSceneRenderer::drawScene()
{
    m_pRenderer->prepareFrame();
    VkExtent2D extent{
        .width = m_pRenderer->getWindowWidth(),
        .height = m_pRenderer->getWindowHeight(),
    };
    VkViewport viewport = vkl::init::viewport(extent);
    VkRect2D scissor = vkl::init::rect2D(extent);

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.pRenderPass = m_forwardPass.renderPass;
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = extent;
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    auto commandIndex = m_pRenderer->getCurrentFrameIndex();
    auto *commandBuffer = m_pRenderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->cmdSetViewport(&viewport);
    commandBuffer->cmdSetSissor(&scissor);

    // forward pass
    commandBuffer->cmdBindPipeline(m_forwardPass.pipeline);
    commandBuffer->cmdBindDescriptorSet(m_forwardPass.pipeline, PASS_FORWARD::SET_SCENE, 1, &m_sceneSets[commandIndex]);
    commandBuffer->cmdBindDescriptorSet(m_forwardPass.pipeline, PASS_FORWARD::SET_SAMPLER, 1, &m_sampler.set);
    renderPassBeginInfo.pFramebuffer = m_forwardPass.framebuffers[m_pRenderer->getCurrentImageIndex()];
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);
    for(auto &renderable : m_renderList)
    {
        _drawRenderData(renderable, m_forwardPass.pipeline, commandBuffer);
    }
    commandBuffer->cmdEndRenderPass();

    renderPassBeginInfo.pFramebuffer = m_postFxPass.framebuffers[m_pRenderer->getCurrentImageIndex()];
    renderPassBeginInfo.pRenderPass = m_postFxPass.renderPass;
    renderPassBeginInfo.clearValueCount = 1;
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);
    VkDeviceSize offsets[1] = { 0 };
    commandBuffer->cmdBindVertexBuffers(0, 1, m_postFxPass.quadVB, offsets);
    commandBuffer->cmdBindPipeline(m_postFxPass.pipeline);
    commandBuffer->cmdBindDescriptorSet(m_postFxPass.pipeline, PASS_POSTFX::SET_OFFSCREEN, 1,
                                        &m_postFxPass.sets[m_pRenderer->getCurrentImageIndex()]);
    commandBuffer->cmdBindDescriptorSet(m_postFxPass.pipeline, PASS_POSTFX::SET_SAMPLER, 1, &m_sampler.set);
    commandBuffer->cmdDraw(6, 1, 0, 0);
    commandBuffer->cmdEndRenderPass();

    commandBuffer->end();
    m_pRenderer->submitAndPresent();
}

void VulkanSceneRenderer::update(float deltaTime)
{
    for(auto &ubo : m_uniformList)
    {
        ubo->m_buffer->copyTo(ubo->m_object->getData());
    }
}

void VulkanSceneRenderer::_initRenderData()
{
    {
        BufferCreateInfo bufferCI{
            .size = sizeof(SceneInfo),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_COHERENT_BIT | MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &m_sceneInfoUB, &m_sceneInfo));
        m_sceneInfoUB->setupDescriptor();
    }

    for(auto &set : m_sceneSets)
    {
        std::vector<VkWriteDescriptorSet> writes{
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &m_sceneInfoUB->getBufferInfo()),
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_cameraInfos.data(),
                                          m_cameraInfos.size()),
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, m_lightInfos.data(),
                                          m_lightInfos.size()),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    for(auto &renderable : m_renderList)
    {
        {
            ObjectInfo objInfo{};
            BufferCreateInfo bufferCI{
                .size = sizeof(ObjectInfo),
                .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
            };
            VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &renderable->m_objectUB, &objInfo));
            renderable->m_objectUB->setupDescriptor();
        }

        {
            renderable->m_objectSet =
                m_forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_OBJECT)->allocateSet();

            std::vector<VkWriteDescriptorSet> descriptorWrites{ vkl::init::writeDescriptorSet(
                renderable->m_objectSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                &renderable->m_objectUB->getBufferInfo()) };

            vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
        }

        for (auto & texture : m_textures){
            texture.descriptorInfo = {
                .sampler = m_sampler.texture,
                .imageView = texture.imageView->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
        }

        for(auto &material : m_scene->getMaterials())
        {
            // write descriptor set
            auto set = m_forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_MATERIAL)->allocateSet();
            VulkanBuffer *matInfoUB = nullptr;
            {
                {
                    BufferCreateInfo bufferCI{
                        .size = sizeof(Material),
                        .alignment = 0,
                        .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    };
                    m_pDevice->createBuffer(&bufferCI, &matInfoUB, &material);
                    matInfoUB->setupDescriptor();
                }
                std::vector<VkWriteDescriptorSet> descriptorWrites{};
                // create material buffer Info
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                                                         &matInfoUB->getBufferInfo()));

                auto bindingBits = MATERIAL_BINDING_PBR;
                if(bindingBits & MATERIAL_BINDING_BASECOLOR)
                {
                    std::cerr << "material id: [" << material->id << "] [base color]: ";
                    if(material->baseColorTextureIndex > -1)
                    {
                        descriptorWrites.push_back(
                            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1,
                                                          &m_textures[material->baseColorTextureIndex].descriptorInfo));
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
                        descriptorWrites.push_back(
                            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2,
                                                          &m_textures[material->normalTextureIndex].descriptorInfo));
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
                        descriptorWrites.push_back(vkl::init::writeDescriptorSet(
                            set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3,
                            &m_textures[material->metallicRoughnessTextureIndex].descriptorInfo));
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
                        descriptorWrites.push_back(
                            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4,
                                                          &m_textures[material->occlusionTextureIndex].descriptorInfo));
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
                        descriptorWrites.push_back(
                            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5,
                                                          &m_textures[material->emissiveTextureIndex].descriptorInfo));
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

            m_materialDataMaps[material] = { matInfoUB, set };
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

        auto texture = createTexture(m_pDevice, width, height, imageData, imageDataSize, true);
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
            auto renderable = std::make_shared<VulkanRenderData>(m_pDevice, node);
            m_renderList.push_back(renderable);
        }
        break;
        case ObjectType::CAMERA:
        {
            auto ubo = std::make_shared<VulkanUniformData>(m_pDevice, node);
            m_uniformList.push_front(ubo);
            m_cameraInfos.push_back(ubo->m_buffer->getBufferInfo());
            m_sceneInfo.cameraCount++;
        }
        break;
        case ObjectType::LIGHT:
        {
            auto ubo = std::make_shared<VulkanUniformData>(m_pDevice, node);
            m_uniformList.push_back(ubo);
            m_lightInfos.push_back(ubo->m_buffer->getBufferInfo());
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
    // VulkanPipeline *pipeline = m_forwardPass.pipeline;
    // VkDescriptorSet set = m_skyboxResource.set;
    // VulkanImage *cubemap = m_skyboxResource.cubeMap;

    // {
    //     // auto             texture       = loadCubemapFromFile("");
    //     // VulkanBuffer    *stagingBuffer = nullptr;
    //     // BufferCreateInfo bufferCI{
    //     //     .size      = static_cast<uint32_t>(texture->data.size()),
    //     //     .alignment = 0,
    //     //     .usage     = BUFFER_USAGE_TRANSFER_SRC_BIT,
    //     //     .property  = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //     // };
    //     // VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &stagingBuffer));
    //     // stagingBuffer->map();
    //     // stagingBuffer->copyTo(texture->data.data(), texture->data.size());
    //     // stagingBuffer->unmap();

    //     // ImageCreateInfo imageCI{
    //     //     .extent      = {texture->width, texture->height, 1},
    //     //     .flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
    //     //     .imageType   = IMAGE_TYPE_2D,
    //     //     .arrayLayers = 6,
    //     //     .usage       = IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_DST_BIT,
    //     //     .property    = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //     //     .format      = FORMAT_B8G8R8A8_UNORM,
    //     //     .tiling      = IMAGE_TILING_OPTIMAL,
    //     // };

    //     // VK_CHECK_RESULT(m_pDevice->createImage(&imageCI, &cubemap));

    //     // auto *cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
    //     // cmd->cmdTransitionImageLayout(cubemap, VK_IMAGE_LAYOUT_UNDEFINED,
    //     // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL); cmd->cmdCopyBufferToImage(stagingBuffer, cubemap);
    //     // cmd->cmdTransitionImageLayout(cubemap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); m_pDevice->endSingleTimeCommands(cmd);

    //     // ImageViewCreateInfo imageViewCI{
    //     //     .viewType  = IMAGE_VIEW_TYPE_CUBE,
    //     //     .dimension = IMAGE_VIEW_DIMENSION_CUBE,
    //     //     .format    = imageCI.format,
    //     // };

    //     // VkImageSubresourceRange subresourceRange = {};
    //     // subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    //     // subresourceRange.baseMipLevel            = 0;
    //     // subresourceRange.levelCount              = cubemap->getMipLevels();
    //     // subresourceRange.layerCount              = 6;

    //     // _skyboxResource.cubeMapDescInfo = {
    //     //     .sampler     = _skyboxResource.cubeMapSampler,
    //     //     .imageView   = _skyboxResource.cubeMapView->getHandle(),
    //     //     .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //     // };
    // }

    // pipeline = CreateSkyboxPipeline(m_pDevice, m_pRenderer->getDefaultRenderPass());
    // set = pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_SKYBOX)->allocateSet();

    // std::vector<VkWriteDescriptorSet> writeDescriptorSets{
    //     // Binding 0 : Vertex shader uniform buffer
    //     vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
    //     m_cameraInfos.data()),
    //     // Binding 1 : Fragment shader cubemap sampler
    //     vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
    //                                   &m_skyboxResource.cubeMapDescInfo)
    // };

    // vkUpdateDescriptorSets(m_pDevice->getHandle(), writeDescriptorSets.size(),
    //                        writeDescriptorSets.data(), 0, nullptr);
}

void VulkanSceneRenderer::_initPostFx()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    m_postFxPass.colorImages.resize(imageCount);
    m_postFxPass.colorImageViews.resize(imageCount);
    m_postFxPass.framebuffers.resize(imageCount);
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

        VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &m_postFxPass.quadVB, quadVertices));
    }

    // color attachment
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto &colorImage = m_postFxPass.colorImages[idx];
        auto &colorImageView = m_postFxPass.colorImageViews[idx];
        auto &framebuffer = m_postFxPass.framebuffers[idx];

        // get swapchain image
        {
            colorImage = m_pRenderer->getSwapChain()->getImage(idx);
        }

        // get image view
        {
            ImageViewCreateInfo createInfo{};
            createInfo.format = FORMAT_B8G8R8A8_UNORM;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            m_pDevice->createImageView(&createInfo, &colorImageView, colorImage);
        }

        {
            std::vector<VulkanImageView *> attachments{ colorImageView };
            FramebufferCreateInfo createInfo{};
            createInfo.width = m_pRenderer->getSwapChain()->getExtent().width;
            createInfo.height = m_pRenderer->getSwapChain()->getExtent().height;
            VK_CHECK_RESULT(
                m_pDevice->createFramebuffers(&createInfo, &framebuffer, attachments.size(), attachments.data()));
        }
    }

    {
        VkAttachmentDescription colorAttachment{
            .format = m_pRenderer->getSwapChain()->getImageFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

        };

        std::vector<VkAttachmentDescription> colorAttachments{
            colorAttachment,
        };

        VK_CHECK_RESULT(m_pDevice->createRenderPass(nullptr, &m_postFxPass.renderPass, colorAttachments));
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        EffectInfo effectInfo{};
        effectInfo.setLayouts.push_back(m_setLayout.pOffScreen);
        effectInfo.setLayouts.push_back(m_setLayout.pSampler);
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT] =
            m_pDevice->getShaderCache()->getShaders(shaderDir / "postFX.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] =
            m_pDevice->getShaderCache()->getShaders(shaderDir / "postFX.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
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
        VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo,
                                                          m_postFxPass.renderPass, &m_postFxPass.pipeline));
    }

    for(uint32_t idx = 0; idx < imageCount; idx++)
    {
        auto &set = m_postFxPass.sets[idx];
        set = m_postFxPass.pipeline->getDescriptorSetLayout(PASS_POSTFX::SET_OFFSCREEN)->allocateSet();

        VkDescriptorImageInfo imageInfo{
            .sampler = m_sampler.postFX,
            .imageView = m_forwardPass.colorImageViews[idx]->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        std::vector<VkWriteDescriptorSet> writes{
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, &imageInfo),
        };

        vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void VulkanSceneRenderer::_initForward()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    VkExtent2D imageExtent = m_pRenderer->getSwapChain()->getExtent();

    m_forwardPass.framebuffers.resize(imageCount);
    m_forwardPass.colorImages.resize(imageCount);
    m_forwardPass.colorImageViews.resize(imageCount);
    m_forwardPass.depthImages.resize(imageCount);
    m_forwardPass.depthImageViews.resize(imageCount);

    // render pass
    {
        VkAttachmentDescription colorAttachment{
            .format = m_pRenderer->getSwapChain()->getImageFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

        };
        VkAttachmentDescription depthAttachment{
            .format = m_pDevice->getDepthFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        std::vector<VkAttachmentDescription> colorAttachments{
            colorAttachment,
        };

        VK_CHECK_RESULT(
            m_pDevice->createRenderPass(nullptr, &m_forwardPass.renderPass, colorAttachments, depthAttachment));
    }

    // frame buffer
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto &colorImage = m_forwardPass.colorImages[idx];
        auto &colorImageView = m_forwardPass.colorImageViews[idx];
        auto &depthImage = m_forwardPass.depthImages[idx];
        auto &depthImageView = m_forwardPass.depthImageViews[idx];
        auto &framebuffer = m_forwardPass.framebuffers[idx];

        {
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .imageType = IMAGE_TYPE_2D,
                .usage = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_SAMPLED_BIT,
                .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImage(&createInfo, &colorImage);
        }

        {
            ImageViewCreateInfo createInfo{
                .viewType = IMAGE_VIEW_TYPE_2D,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImageView(&createInfo, &colorImageView, colorImage);
        }

        {
            VkFormat depthFormat = m_pDevice->getDepthFormat();
            ImageCreateInfo createInfo{};
            createInfo.extent = { imageExtent.width, imageExtent.height, 1 };
            createInfo.format = static_cast<Format>(depthFormat);
            createInfo.tiling = IMAGE_TILING_OPTIMAL;
            createInfo.usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK_RESULT(m_pDevice->createImage(&createInfo, &depthImage));
        }

        VulkanCommandBuffer *cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdTransitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        m_pDevice->endSingleTimeCommands(cmd);

        {
            ImageViewCreateInfo createInfo{};
            createInfo.format = FORMAT_D32_SFLOAT;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            VK_CHECK_RESULT(m_pDevice->createImageView(&createInfo, &depthImageView, depthImage));
        }

        {
            std::vector<VulkanImageView *> attachments{ colorImageView, depthImageView };
            FramebufferCreateInfo createInfo{};
            createInfo.width = imageExtent.width;
            createInfo.height = imageExtent.height;
            VK_CHECK_RESULT(
                m_pDevice->createFramebuffers(&createInfo, &framebuffer, attachments.size(), attachments.data()));
        }
    }

    {
        EffectInfo effectInfo{};
        auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        effectInfo.setLayouts = { m_setLayout.pScene, m_setLayout.pObject, m_setLayout.pMaterial,
                                  m_setLayout.pSampler };
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, m_pDevice->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_pDevice->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv") },
        };

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        VK_CHECK_RESULT(m_pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, m_forwardPass.renderPass,
                                                          &m_forwardPass.pipeline));
    }

    m_sceneSets.resize(m_pRenderer->getCommandBufferCount());
    for(auto &set : m_sceneSets)
    {
        set = m_forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_SCENE)->allocateSet();
    }
}

void VulkanSceneRenderer::_initShadow()
{
    // uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();

    // m_shadowPass.framebuffers.resize(imageCount);
    // m_shadowPass.depthImageViews.resize(imageCount);
    // m_shadowPass.depthImages.resize(imageCount);

    // for(uint32_t idx = 0; idx < imageCount; idx++)
    // {
    //     auto &framebuffer = m_shadowPass.framebuffers[idx];
    //     auto &depthImage = m_shadowPass.depthImages[idx];
    //     auto &depthImageView = m_shadowPass.depthImageViews[idx];

    //     {
    //         VkFormat depthFormat = m_pDevice->getDepthFormat();
    //         ImageCreateInfo createInfo{};
    //         createInfo.extent = { m_shadowPass.dim, m_shadowPass.dim, 1 };
    //         createInfo.format = static_cast<Format>(depthFormat);
    //         createInfo.tiling = IMAGE_TILING_OPTIMAL;
    //         createInfo.usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    //         createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    //         VK_CHECK_RESULT(m_pDevice->createImage(&createInfo, &depthImage));
    //     }

    //     VulkanCommandBuffer *cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
    //     cmd->cmdTransitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
    //                                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    //     m_pDevice->endSingleTimeCommands(cmd);

    //     {
    //         ImageViewCreateInfo createInfo{};
    //         createInfo.format = FORMAT_D32_SFLOAT;
    //         createInfo.viewType = IMAGE_VIEW_TYPE_2D;
    //         VK_CHECK_RESULT(m_pDevice->createImageView(&createInfo, &depthImageView, depthImage));
    //     }

    //     {
    //         std::vector<VulkanImageView *> attachments{ depthImageView };
    //         FramebufferCreateInfo createInfo{};
    //         createInfo.width = m_shadowPass.dim;
    //         createInfo.height = m_shadowPass.dim;
    //         VK_CHECK_RESULT(m_pDevice->createFramebuffers(&createInfo, &framebuffer,
    //         attachments.size(),
    //                                                       attachments.data()));
    //     }
    // }

    // {
    //     VkAttachmentDescription depthAttachment{
    //         .format = m_pDevice->getDepthFormat(),
    //         .samples = VK_SAMPLE_COUNT_1_BIT,
    //         .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    //         .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    //         .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    //         .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    //         .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //         .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //     };

    //     VK_CHECK_RESULT(m_pDevice->createRenderPass(nullptr, &m_shadowPass.renderPass,
    //     depthAttachment));
    // }

    // m_shadowPass.pipeline = CreateShadowPipeline(m_pDevice, m_shadowPass.renderPass);

    // m_shadowPass.cameraSets.resize(imageCount);
    // for(auto &set : m_shadowPass.cameraSets)
    // {
    //     set = m_shadowPass.pipeline->getDescriptorSetLayout(0)->allocateSet();
    // }
}
void VulkanSceneRenderer::_drawRenderData(const std::shared_ptr<VulkanRenderData> &renderData, VulkanPipeline *pipeline,
                                          VulkanCommandBuffer *drawCmd)
{
    auto mesh = renderData->m_node->getObject<Mesh>();
    drawCmd->cmdPushConstants(pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                              &renderData->m_node->matrix);
    VkDeviceSize offsets[1] = { 0 };
    drawCmd->cmdBindVertexBuffers(0, 1, renderData->m_vertexBuffer, offsets);
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
        drawCmd->cmdBindIndexBuffers(renderData->m_indexBuffer, 0, indexType);
    }
    for(const auto &subset : mesh->m_subsets)
    {
        if(subset.indexCount > 0)
        {
            auto &material = m_scene->getMaterials()[subset.materialIndex];
            auto &materialSet = m_materialDataMaps[material];
            drawCmd->cmdBindDescriptorSet(pipeline, 2, 1, &materialSet.set);
            if(subset.hasIndices)
            {
                drawCmd->cmdDrawIndexed(subset.indexCount, 1, subset.firstIndex, 0, 0);
            }
            else
            {
                drawCmd->cmdDraw(subset.vertexCount, 1, subset.firstVertex, 0);
            }
        }
    }
}

void VulkanSceneRenderer::_initSampler()
{
    // texture
    {
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.maxLod = calculateFullMipLevels(2048, 2048);
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &samplerInfo, nullptr, &m_sampler.texture));
    }

    // shadow
    {
        VkSamplerCreateInfo createInfo = vkl::init::samplerCreateInfo();
        createInfo.magFilter = m_shadowPass.filter;
        createInfo.minFilter = m_shadowPass.filter;
        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &m_sampler.shadow));
    }

    // postFX
    {
        VkSamplerCreateInfo createInfo = vkl::init::samplerCreateInfo();
        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &m_sampler.postFX));
    }

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
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_SAMPLER, 0, imageInfos.data(), imageInfos.size()),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}
void VulkanSceneRenderer::_initSetLayout()
{
    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1,
                                                  m_sceneInfo.cameraCount),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2,
                                                  m_sceneInfo.lightCount),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pScene);
    }

    // off screen texture
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pOffScreen);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                  0),  // material info
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pMaterial);
    }

    // object
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pObject);
    }

    // sampler
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(&createInfo, &m_setLayout.pSampler);
    }
}
}  // namespace vkl
