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
#include "scene/entity.h"
#include "scene/light.h"
#include "scene/sceneNode.h"
#include "swapChain.h"
#include "vkInit.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkanRenderer.h"

namespace vkl {

namespace {

VulkanPipeline *CreateShadowPipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass) {
    VulkanPipeline            *pipeline;
    VulkanDescriptorSetLayout *sceneLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        EffectInfo            effectInfo{};
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "shadow.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "shadow.frag.spv");

        GraphicsPipelineCreateInfo                   pipelineCreateInfo{};
        pipelineCreateInfo.vertexInputInfo = pipelineCreateInfo.vertexInputBuilder.getPipelineVertexInputState({VertexComponent::POSITION});
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }

    return pipeline;
}

VulkanPipeline *CreatePostFxPipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass) {
    VulkanPipeline            *pipeline;
    VulkanDescriptorSetLayout *textureLayout = nullptr;

    // off screen texture
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &textureLayout);
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        EffectInfo            effectInfo{};
        effectInfo.setLayouts.push_back(textureLayout);
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "postFX.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "postFX.frag.spv");

        GraphicsPipelineCreateInfo                   pipelineCreateInfo{};
        std::vector<VkVertexInputBindingDescription> bindingDescs{
            {0, 4 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX},
        };
        std::vector<VkVertexInputAttributeDescription> attrDescs{
            {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
            {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)}};
        pipelineCreateInfo.vertexInputInfo = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescs.size()),
            .pVertexBindingDescriptions      = bindingDescs.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size()),
            .pVertexAttributeDescriptions    = attrDescs.data(),
        };
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }

    return pipeline;
}

VulkanPipeline *CreateSkyboxPipeline(VulkanDevice *pDevice, VulkanRenderPass *pRenderPass) {
    VulkanPipeline            *pipeline;
    VulkanDescriptorSetLayout *sceneLayout    = nullptr;
    VulkanDescriptorSetLayout *materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.push_back(vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0));
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        // build Shader
        std::filesystem::path shaderDir = "assets/shaders/glsl/default";
        EffectInfo            effectInfo{};
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "skybox.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "skybox.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo;
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }

    return pipeline;
}

VulkanPipeline *CreateForwardPipeline(VulkanDevice *pDevice,
                                      VulkanRenderPass *pRenderPass,
                                      SceneInfo &sceneInfo) {
    VulkanPipeline            *pipeline       = nullptr;
    VulkanDescriptorSetLayout *sceneLayout    = nullptr;
    VulkanDescriptorSetLayout *objectLayout   = nullptr;
    VulkanDescriptorSetLayout *materialLayout = nullptr;

    // scene
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, sceneInfo.cameraCount),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2, sceneInfo.lightCount),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &sceneLayout);
    }

    // object
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &objectLayout);
    }

    // material
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0), // material info
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
            vkl::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = vkl::init::descriptorSetLayoutCreateInfo(bindings);
        pDevice->createDescriptorSetLayout(&createInfo, &materialLayout);
    }

    {
        EffectInfo            effectInfo{};
        std::filesystem::path shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        effectInfo.setLayouts.push_back(sceneLayout);
        effectInfo.setLayouts.push_back(objectLayout);
        effectInfo.setLayouts.push_back(materialLayout);
        effectInfo.constants.push_back(vkl::init::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0));
        effectInfo.shaderMapList[VK_SHADER_STAGE_VERTEX_BIT]   = pDevice->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv");
        effectInfo.shaderMapList[VK_SHADER_STAGE_FRAGMENT_BIT] = pDevice->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv");

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        VK_CHECK_RESULT(pDevice->createGraphicsPipeline(&pipelineCreateInfo, &effectInfo, pRenderPass, &pipeline));
    }
    return pipeline;
}

} // namespace

namespace {
} // namespace

std::unique_ptr<VulkanSceneRenderer> VulkanSceneRenderer::Create(const std::shared_ptr<VulkanRenderer> &renderer) {
    auto instance = std::make_unique<VulkanSceneRenderer>(renderer);
    return instance;
}

VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer)
    : m_pDevice(renderer->getDevice()),
      m_pRenderer(renderer) {
}

void VulkanSceneRenderer::loadResources() {
    // _initPostFxResource();
    _loadSceneNodes();
    _initForwardResource();
    _initPostFxResource();
    // _initShadowPassResource();
    _initRenderData();
}

void VulkanSceneRenderer::cleanupResources() {
    if (_forwardPass.pipeline != nullptr) {
        m_pDevice->destroyPipeline(_forwardPass.pipeline);
    }

    if (_postFxPass.pipeline != nullptr) {
        m_pDevice->destroyPipeline(_postFxPass.pipeline);
    }

    for (uint32_t idx = 0; idx < m_pRenderer->getSwapChain()->getImageCount(); idx++) {
        m_pDevice->destroyFramebuffers(_postFxPass.framebuffers[idx]);
        m_pDevice->destroyFramebuffers(_forwardPass.framebuffers[idx]);
        m_pDevice->destroyImage(_forwardPass.colorImages[idx]);
        m_pDevice->destroyImageView(_postFxPass.colorImageViews[idx]);
        m_pDevice->destroyImageView(_forwardPass.colorImageViews[idx]);
        m_pDevice->destroyImage(_forwardPass.depthImages[idx]);
        m_pDevice->destroyImageView(_forwardPass.depthImageViews[idx]);
    }
    vkDestroySampler(m_pDevice->getHandle(), _postFxPass.sampler, nullptr);
    m_pDevice->destroyBuffer(_postFxPass.quadVB);
    m_pDevice->destroyBuffer(_sceneInfoUB);
    vkDestroyRenderPass(m_pDevice->getHandle(), _forwardPass.renderPass->getHandle(), nullptr);
    vkDestroyRenderPass(m_pDevice->getHandle(), _postFxPass.renderPass->getHandle(), nullptr);
}

void VulkanSceneRenderer::drawScene() {
    m_pRenderer->prepareFrame();
    VkExtent2D extent{
        .width  = m_pRenderer->getWindowWidth(),
        .height = m_pRenderer->getWindowHeight(),
    };
    VkViewport viewport = vkl::init::viewport(extent);
    VkRect2D   scissor  = vkl::init::rect2D(extent);

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.pRenderPass       = _forwardPass.renderPass;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = extent;
    renderPassBeginInfo.clearValueCount   = clearValues.size();
    renderPassBeginInfo.pClearValues      = clearValues.data();

    VkCommandBufferBeginInfo beginInfo = vkl::init::commandBufferBeginInfo();

    auto  commandIndex  = m_pRenderer->getCurrentFrameIndex();
    auto *commandBuffer = m_pRenderer->getDefaultCommandBuffer(commandIndex);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->cmdSetViewport(&viewport);
    commandBuffer->cmdSetSissor(&scissor);
    commandBuffer->cmdBindPipeline(_forwardPass.pipeline);
    commandBuffer->cmdBindDescriptorSet(_forwardPass.pipeline, 0, 1, &_sceneSets[commandIndex]);

    // forward pass
    renderPassBeginInfo.pFramebuffer = _forwardPass.framebuffers[m_pRenderer->getCurrentImageIndex()];
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);
    for (auto &renderable : _renderList) {
        VkDeviceSize offsets[1] = {0};
        commandBuffer->cmdBindVertexBuffers(0, 1, renderable->m_vertexBuffer, offsets);
        commandBuffer->cmdBindIndexBuffers(renderable->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        _drawNodes(renderable, _forwardPass.pipeline, commandBuffer, renderable->m_node->getObject<Entity>()->m_rootNode);
    }
    commandBuffer->cmdEndRenderPass();

    renderPassBeginInfo.pFramebuffer    = _postFxPass.framebuffers[m_pRenderer->getCurrentImageIndex()];
    renderPassBeginInfo.pRenderPass     = _postFxPass.renderPass;
    renderPassBeginInfo.clearValueCount = 1;
    commandBuffer->cmdBeginRenderPass(&renderPassBeginInfo);
    VkDeviceSize offsets[1] = {0};
    commandBuffer->cmdBindVertexBuffers(0, 1, _postFxPass.quadVB, offsets);
    commandBuffer->cmdBindPipeline(_postFxPass.pipeline);
    commandBuffer->cmdBindDescriptorSet(_postFxPass.pipeline, 0, 1, &_postFxPass.sets[m_pRenderer->getCurrentImageIndex()]);
    commandBuffer->cmdDraw(6, 1, 0, 0);
    commandBuffer->cmdEndRenderPass();

    commandBuffer->end();
    m_pRenderer->submitAndPresent();
}

void VulkanSceneRenderer::update(float deltaTime) {
    for (auto &ubo : _uniformList) {
        ubo->m_buffer->copyTo(ubo->m_object->getData());
    }
}

void VulkanSceneRenderer::_initRenderData() {
    {
        BufferCreateInfo bufferCI{
            .size      = sizeof(SceneInfo),
            .alignment = 0,
            .usage     = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property  = MEMORY_PROPERTY_HOST_COHERENT_BIT | MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &_sceneInfoUB, &_sceneInfo));
        _sceneInfoUB->setupDescriptor();
    }

    for (auto &set : _sceneSets) {
        std::vector<VkWriteDescriptorSet> writes{
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &_sceneInfoUB->getBufferInfo()),
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, _cameraInfos.data(), _cameraInfos.size()),
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, _lightInfos.data(), _lightInfos.size()),
        };
        vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
    }

    for (auto &renderable : _renderList) {
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
            renderable->m_objectSet = _forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_OBJECT)->allocateSet();

            std::vector<VkWriteDescriptorSet> descriptorWrites{
                vkl::init::writeDescriptorSet(renderable->m_objectSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &renderable->m_objectUB->getBufferInfo())
            };

            vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

    for (auto &material : renderable->m_node->getObject<Entity>()->m_materials) {
        // write descriptor set
        auto set = _forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_MATERIAL)->allocateSet();
        VulkanBuffer * matInfoUB = nullptr;
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
            descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matInfoUB->getBufferInfo()));

            auto bindingBits = MATERIAL_BINDING_PBR;
            auto &m_textures = renderable->m_textures;
            if (bindingBits & MATERIAL_BINDING_BASECOLOR) {
                std::cerr << "material id: [" << material->id << "] [base color]: ";
                if (material->baseColorTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &m_textures[material->baseColorTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                }
                else {
                    std::cerr << "texture not found." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_NORMAL) {
                std::cerr << "material id: [" << material->id << "] [normal]: ";
                if (material->normalTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &m_textures[material->normalTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    std::cerr << "texture not found." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_PHYSICAL){
                std::cerr << "material id: [" << material->id << "] [physical desc]: ";
                if (material->metallicFactor > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &m_textures[material->metallicRoughnessTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    std::cerr << "texture not found." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_AO){
                std::cerr << "material id: [" << material->id << "] [ao]: ";
                if (material->occlusionTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &m_textures[material->occlusionTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    std::cerr << "texture not found." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_EMISSIVE){
                std::cerr << "material id: [" << material->id << "] [emissive]: ";
                if (material->emissiveTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &m_textures[material->emissiveTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    std::cerr << "texture not found." << std::endl;
                }
            }

            vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        materiaDataMaps[material] = {matInfoUB, set};
    }
    }
}

void VulkanSceneRenderer::_loadSceneNodes() {
    _sceneInfo.ambient = glm::vec4(_scene->getAmbient(), 1.0f);
    std::queue<std::shared_ptr<SceneNode>> q;
    q.push(_scene->getRootNode());

    while (!q.empty()) {
        auto node = q.front();
        q.pop();

        switch (node->attachType) {
        case AttachType::ENTITY: {
            auto renderable = std::make_shared<VulkanRenderData>(m_pDevice, node);
            _renderList.push_back(renderable);
        } break;
        case AttachType::CAMERA: {
            auto ubo = std::make_shared<VulkanUniformData>(m_pDevice, node);
            _uniformList.push_front(ubo);
            _cameraInfos.push_back(ubo->m_buffer->getBufferInfo());
            _sceneInfo.cameraCount++;
        } break;
        case AttachType::LIGHT: {
            auto ubo = std::make_shared<VulkanUniformData>(m_pDevice, node);
            _uniformList.push_back(ubo);
            _lightInfos.push_back(ubo->m_buffer->getBufferInfo());
            _sceneInfo.lightCount++;
        } break;
        default:
            assert("unattached scene node.");
            break;
        }

        for (const auto &subNode : node->children) {
            q.push(subNode);
        }
    }
}

void VulkanSceneRenderer::_initSkyboxResource() {
    VulkanPipeline *pipeline = _forwardPass.pipeline;
    VkDescriptorSet set      = _skyboxResource.set;
    VulkanImage    *cubemap  = _skyboxResource.cubeMap;

    {
        // auto             texture       = loadCubemapFromFile("");
        // VulkanBuffer    *stagingBuffer = nullptr;
        // BufferCreateInfo bufferCI{
        //     .size      = static_cast<uint32_t>(texture->data.size()),
        //     .alignment = 0,
        //     .usage     = BUFFER_USAGE_TRANSFER_SRC_BIT,
        //     .property  = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        // };
        // VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &stagingBuffer));
        // stagingBuffer->map();
        // stagingBuffer->copyTo(texture->data.data(), texture->data.size());
        // stagingBuffer->unmap();

        // ImageCreateInfo imageCI{
        //     .extent      = {texture->width, texture->height, 1},
        //     .flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        //     .imageType   = IMAGE_TYPE_2D,
        //     .arrayLayers = 6,
        //     .usage       = IMAGE_USAGE_SAMPLED_BIT | IMAGE_USAGE_TRANSFER_DST_BIT,
        //     .property    = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        //     .format      = FORMAT_B8G8R8A8_UNORM,
        //     .tiling      = IMAGE_TILING_OPTIMAL,
        // };

        // VK_CHECK_RESULT(m_pDevice->createImage(&imageCI, &cubemap));

        // auto *cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        // cmd->cmdTransitionImageLayout(cubemap, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        // cmd->cmdCopyBufferToImage(stagingBuffer, cubemap);
        // cmd->cmdTransitionImageLayout(cubemap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // m_pDevice->endSingleTimeCommands(cmd);

        // ImageViewCreateInfo imageViewCI{
        //     .viewType  = IMAGE_VIEW_TYPE_CUBE,
        //     .dimension = IMAGE_VIEW_DIMENSION_CUBE,
        //     .format    = imageCI.format,
        // };

        // VkImageSubresourceRange subresourceRange = {};
        // subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        // subresourceRange.baseMipLevel            = 0;
        // subresourceRange.levelCount              = cubemap->getMipLevels();
        // subresourceRange.layerCount              = 6;

        // _skyboxResource.cubeMapDescInfo = {
        //     .sampler     = _skyboxResource.cubeMapSampler,
        //     .imageView   = _skyboxResource.cubeMapView->getHandle(),
        //     .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        // };
    }

    pipeline = CreateSkyboxPipeline(m_pDevice, m_pRenderer->getDefaultRenderPass());
    set      = pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_SKYBOX)->allocateSet();

    std::vector<VkWriteDescriptorSet> writeDescriptorSets{
        // Binding 0 : Vertex shader uniform buffer
        vkl::init::writeDescriptorSet(
            set,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            _cameraInfos.data()),
        // Binding 1 : Fragment shader cubemap sampler
        vkl::init::writeDescriptorSet(
            set,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            &_skyboxResource.cubeMapDescInfo)};

    vkUpdateDescriptorSets(m_pDevice->getHandle(), writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void VulkanSceneRenderer::_initPostFxResource() {
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    _postFxPass.colorImages.resize(imageCount);
    _postFxPass.colorImageViews.resize(imageCount);
    _postFxPass.framebuffers.resize(imageCount);
    _postFxPass.sets.resize(imageCount);

    // buffer
    {
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f};

        BufferCreateInfo bufferCI{
            .size      = sizeof(quadVertices),
            .alignment = 0,
            .usage     = BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .property  = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &_postFxPass.quadVB, quadVertices));
    }

    // sampler
    {
        VkSamplerCreateInfo createInfo = vkl::init::samplerCreateInfo();
        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &_postFxPass.sampler));
    }

    // color attachment
    for (auto idx = 0; idx < imageCount; idx++) {
        auto &colorImage     = _postFxPass.colorImages[idx];
        auto &colorImageView = _postFxPass.colorImageViews[idx];
        auto &framebuffer    = _postFxPass.framebuffers[idx];

        // get swapchain image
        {
            colorImage = m_pRenderer->getSwapChain()->getImage(idx);
        }

        // get image view
        {
            ImageViewCreateInfo createInfo{};
            createInfo.format   = FORMAT_B8G8R8A8_UNORM;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            m_pDevice->createImageView(&createInfo, &colorImageView, colorImage);
        }


        {
            std::vector<VulkanImageView *> attachments{colorImageView};
            FramebufferCreateInfo          createInfo{};
            createInfo.width  = m_pRenderer->getSwapChain()->getExtent().width;
            createInfo.height = m_pRenderer->getSwapChain()->getExtent().height;
            VK_CHECK_RESULT(m_pDevice->createFramebuffers(&createInfo, &framebuffer, attachments.size(), attachments.data()));
        }
    }

    {
        VkAttachmentDescription colorAttachment{
            .format         = m_pRenderer->getSwapChain()->getImageFormat(),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

        };

        std::vector<VkAttachmentDescription> colorAttachments{
            colorAttachment,
        };

        VK_CHECK_RESULT(m_pDevice->createRenderPass(nullptr, &_postFxPass.renderPass, colorAttachments));
    }

    _postFxPass.pipeline = CreatePostFxPipeline(m_pDevice, _postFxPass.renderPass);
    for (uint32_t idx = 0; idx < imageCount; idx++) {
        VkDescriptorSet &set = _postFxPass.sets[idx];
        set                  = _postFxPass.pipeline->getDescriptorSetLayout(PASS_POSTFX::SET_OFFSCREEN)->allocateSet();

        VkDescriptorImageInfo imageInfo{
            .sampler     = _postFxPass.sampler,
            .imageView   = _forwardPass.colorImageViews[idx]->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        std::vector<VkWriteDescriptorSet> writes{
            vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageInfo),
        };

        vkUpdateDescriptorSets(m_pDevice->getHandle(),
                               static_cast<uint32_t>(writes.size()),
                               writes.data(), 0, nullptr);
    }
}

void VulkanSceneRenderer::_initForwardResource() {

    uint32_t   imageCount  = m_pRenderer->getSwapChain()->getImageCount();
    VkExtent2D imageExtent = m_pRenderer->getSwapChain()->getExtent();

    _forwardPass.framebuffers.resize(imageCount);
    _forwardPass.colorImages.resize(imageCount);
    _forwardPass.colorImageViews.resize(imageCount);
    _forwardPass.depthImages.resize(imageCount);
    _forwardPass.depthImageViews.resize(imageCount);

    // color attachment
    for (auto idx = 0; idx < imageCount; idx++) {
        auto &colorImage     = _forwardPass.colorImages[idx];
        auto &colorImageView = _forwardPass.colorImageViews[idx];
        auto &depthImage     = _forwardPass.depthImages[idx];
        auto &depthImageView = _forwardPass.depthImageViews[idx];
        auto &framebuffer    = _forwardPass.framebuffers[idx];

        {
            ImageCreateInfo createInfo{
                .extent    = {imageExtent.width, imageExtent.height, 1},
                .imageType = IMAGE_TYPE_2D,
                .usage     = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_SAMPLED_BIT,
                .property  = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format    = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImage(&createInfo, &colorImage);
        }

        {
            ImageViewCreateInfo createInfo{
                .viewType = IMAGE_VIEW_TYPE_2D,
                .format   = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImageView(&createInfo, &colorImageView, colorImage);
        }

        {
            VkFormat        depthFormat = m_pDevice->getDepthFormat();
            ImageCreateInfo createInfo{};
            createInfo.extent   = {imageExtent.width, imageExtent.height, 1};
            createInfo.format   = static_cast<Format>(depthFormat);
            createInfo.tiling   = IMAGE_TILING_OPTIMAL;
            createInfo.usage    = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK_RESULT(m_pDevice->createImage(&createInfo, &depthImage));
        }

        VulkanCommandBuffer *cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdTransitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        m_pDevice->endSingleTimeCommands(cmd);

        {
            ImageViewCreateInfo createInfo{};
            createInfo.format   = FORMAT_D32_SFLOAT;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            VK_CHECK_RESULT(m_pDevice->createImageView(&createInfo, &depthImageView, depthImage));
        }

        {
            std::vector<VulkanImageView *> attachments{colorImageView, depthImageView};
            FramebufferCreateInfo          createInfo{};
            createInfo.width  = imageExtent.width;
            createInfo.height = imageExtent.height;
            VK_CHECK_RESULT(m_pDevice->createFramebuffers(&createInfo, &framebuffer, attachments.size(), attachments.data()));
        }
    }

    {
        VkAttachmentDescription colorAttachment{
            .format         = m_pRenderer->getSwapChain()->getImageFormat(),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

        };
        VkAttachmentDescription depthAttachment{
            .format         = m_pDevice->getDepthFormat(),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        std::vector<VkAttachmentDescription> colorAttachments{
            colorAttachment,
        };

        VK_CHECK_RESULT(m_pDevice->createRenderPass(nullptr, &_forwardPass.renderPass, colorAttachments, depthAttachment));
    }

    _forwardPass.pipeline = CreateForwardPipeline(m_pDevice, _forwardPass.renderPass, _sceneInfo);
    pSceneLayout = _forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_SCENE);
    pPBRMaterialLayout = _forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_MATERIAL);

    _sceneSets.resize(m_pRenderer->getCommandBufferCount());
    for (auto &set : _sceneSets) {
        set = _forwardPass.pipeline->getDescriptorSetLayout(PASS_FORWARD::SET_SCENE)->allocateSet();
    }
}
void VulkanSceneRenderer::_initShadowPassResource() {
    uint32_t   imageCount  = m_pRenderer->getSwapChain()->getImageCount();

    _shadowPass.framebuffers.resize(imageCount);
    _shadowPass.depthImageViews.resize(imageCount);
    _shadowPass.depthImages.resize(imageCount);

    // sampler
    {
        VkSamplerCreateInfo createInfo = vkl::init::samplerCreateInfo();
        createInfo.magFilter = _shadowPass.filter;
        createInfo.minFilter = _shadowPass.filter;
        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &createInfo, nullptr, &_postFxPass.sampler));
    }

    for (uint32_t idx = 0; idx < imageCount; idx++) {
        auto &framebuffer    = _shadowPass.framebuffers[idx];
        auto &depthImage     = _shadowPass.depthImages[idx];
        auto &depthImageView = _shadowPass.depthImageViews[idx];

        {
            VkFormat        depthFormat = m_pDevice->getDepthFormat();
            ImageCreateInfo createInfo{};
            createInfo.extent   = {_shadowPass.dim, _shadowPass.dim, 1};
            createInfo.format   = static_cast<Format>(depthFormat);
            createInfo.tiling   = IMAGE_TILING_OPTIMAL;
            createInfo.usage    = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK_RESULT(m_pDevice->createImage(&createInfo, &depthImage));
        }

        VulkanCommandBuffer *cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdTransitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        m_pDevice->endSingleTimeCommands(cmd);

        {
            ImageViewCreateInfo createInfo{};
            createInfo.format   = FORMAT_D32_SFLOAT;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            VK_CHECK_RESULT(m_pDevice->createImageView(&createInfo, &depthImageView, depthImage));
        }

        {
            std::vector<VulkanImageView *> attachments{depthImageView};
            FramebufferCreateInfo          createInfo{};
            createInfo.width  = _shadowPass.dim;
            createInfo.height = _shadowPass.dim;
            VK_CHECK_RESULT(m_pDevice->createFramebuffers(&createInfo, &framebuffer, attachments.size(), attachments.data()));
        }
    }

    {
        VkAttachmentDescription depthAttachment{
            .format         = m_pDevice->getDepthFormat(),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VK_CHECK_RESULT(m_pDevice->createRenderPass(nullptr, &_shadowPass.renderPass, depthAttachment));
    }

    _shadowPass.pipeline = CreateShadowPipeline(m_pDevice, _shadowPass.renderPass);

    _shadowPass.cameraSets.resize(imageCount);
    for (auto &set : _shadowPass.cameraSets){
        set = _shadowPass.pipeline->getDescriptorSetLayout(0)->allocateSet();
    }
}
void VulkanSceneRenderer::_drawNodes(const std::shared_ptr<VulkanRenderData>& renderData, VulkanPipeline * pipeline, VulkanCommandBuffer * drawCmd, const std::shared_ptr<MeshNode> &node)
{
    std::queue<std::shared_ptr<MeshNode>> q;
    q.push(node);

    while(!q.empty()){
        auto subNode = q.front();
        q.pop();

        glm::mat4 nodeMatrix    = subNode->matrix;
        auto     currentParent = subNode->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        nodeMatrix = renderData->m_node->matrix * nodeMatrix;
        drawCmd->cmdPushConstants(pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);

        for (const auto& subset : subNode->subsets) {
            if (subset.indexCount > 0) {
                auto &material = renderData->m_node->getObject<Entity>()->m_materials[subset.materialIndex];
                auto &materialSet = materiaDataMaps[material];
                drawCmd->cmdBindDescriptorSet(pipeline, 2, 1, &materialSet.set);
                drawCmd->cmdDrawIndexed(subset.indexCount, 1, subset.firstIndex, 0, 0);
            }
        }

        for (const auto &child : subNode->children){
            q.push(child);
        }
    }
}
}  // namespace vkl
