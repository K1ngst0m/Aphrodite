#include "sceneRenderer.h"

namespace vkl {
SceneRenderer::SceneRenderer(SceneManager *scene)
    : _sceneManager(scene) {
}

void SceneRenderer::setScene(SceneManager *scene) {
    _sceneManager = scene;
    prepareResource();
}

VulkanSceneRenderer::VulkanSceneRenderer(SceneManager *scene, VkCommandBuffer commandBuffer, vkl::Device *device, VkQueue graphicsQueue, VkQueue transferQueue)
    : SceneRenderer(scene), _device(device), _drawCmd(commandBuffer), _transferQueue(transferQueue), _graphicsQueue(graphicsQueue) {
}
void VulkanSceneRenderer::prepareResource() {
    _initRenderList();
    _setupDescriptor();
}
void VulkanSceneRenderer::destroy() {
    vkDestroyDescriptorPool(_device->logicalDevice, _descriptorPool, nullptr);
    for (auto * renderable : _renderList){
        for (auto & texture : renderable->_textures){
            texture.destroy();
        }
        renderable->_mesh.destroy();
    }
}
void VulkanSceneRenderer::_initRenderList() {
    for (SceneEntityNode *renderNode : _sceneManager->_renderNodeList) {
        VulkanRenderable * renderable = new VulkanRenderable(this, _device, renderNode->_entity, _drawCmd);
        renderable->shaderPass = renderNode->_pass;
        renderable->transform  = renderNode->_matrix;

        _renderList.push_back(renderable);
    }

    for (auto * renderable : _renderList){
        renderable->loadResouces(_transferQueue);
    }
}
void VulkanSceneRenderer::_setupDescriptor() {
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(_sceneManager->getUBOCount() * _sceneManager->getRenderableCount())},
    };

    uint32_t maxSetSize = _sceneManager->getRenderableCount();

    for (auto & renderable : _renderList) {
        std::vector<VkDescriptorPoolSize> setInfos = renderable->getDescriptorSetInfo();;
        for (auto &setInfo : setInfos) {
            maxSetSize += setInfo.descriptorCount;
            poolSizes.push_back(setInfo);
        }
    }

    VkDescriptorPoolCreateInfo poolInfo = vkl::init::descriptorPoolCreateInfo(poolSizes, maxSetSize);
    VK_CHECK_RESULT(vkCreateDescriptorPool(_device->logicalDevice, &poolInfo, nullptr, &_descriptorPool));

    std::vector<VkDescriptorBufferInfo> bufferInfos{};
    if (_sceneManager->_camera) {
        bufferInfos.push_back(_sceneManager->_camera->_object->buffer.descriptorInfo);
    }
    for (auto *uboNode : _sceneManager->_lightNodeList) {
        bufferInfos.push_back(uboNode->_object->buffer.descriptorInfo);
    }

    for (size_t i = 0; i < _renderList.size(); i++) {
        auto                             *renderNode = _sceneManager->_renderNodeList[i];
        const VkDescriptorSetAllocateInfo allocInfo  = vkl::init::descriptorSetAllocateInfo(_descriptorPool, renderNode->_pass->effect->setLayouts.data(), 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &_renderList[i]->globalDescriptorSet));
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto &bufferInfo : bufferInfos) {
            VkWriteDescriptorSet write = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = _renderList[i]->globalDescriptorSet,
                .dstBinding      = static_cast<uint32_t>(descriptorWrites.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo     = &bufferInfo,
            };
            descriptorWrites.push_back(write);
        }
        vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        _renderList[i]->setupMaterialDescriptor(renderNode->_pass->effect->setLayouts[1], _descriptorPool);
    }
}
vkl::Texture *VulkanRenderable::getTexture(uint32_t index) {
    if (index < _textures.size()) {
        return &_textures[index];
    }
    return nullptr;
}
std::vector<VkDescriptorPoolSize> VulkanRenderable::getDescriptorSetInfo() const {
    std::vector<VkDescriptorPoolSize> poolSizes{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(_textures.size())},
    };
    return poolSizes;
}
void VulkanRenderable::setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) {
    for (auto &material : entity->_materials) {
        VkDescriptorSet             materialSet;
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(descriptorPool, &layout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &materialSet));
        VkWriteDescriptorSet writeDescriptorSet = vkl::init::writeDescriptorSet(materialSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &getTexture(material.baseColorTextureIndex)->descriptorInfo);
        vkUpdateDescriptorSets(_device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
        materialSets.push_back(materialSet);
    }
}
void VulkanRenderable::loadImages(VkQueue queue) {
    for (auto &image : entity->_images) {
        unsigned char *imageData     = image->data;
        uint32_t       imageDataSize = image->dataSize;
        uint32_t       width         = image->width;
        uint32_t       height        = image->height;

        // Load texture from image buffer
        vkl::Buffer stagingBuffer;
        _device->createBuffer(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        stagingBuffer.map();
        stagingBuffer.copyTo(imageData, static_cast<size_t>(imageDataSize));
        stagingBuffer.unmap();

        vkl::Texture texture;
        _device->createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture);

        _device->transitionImageLayout(queue, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        _device->copyBufferToImage(queue, stagingBuffer.buffer, texture.image, width, height);
        _device->transitionImageLayout(queue, texture.image, VK_FORMAT_R8G8B8A8_SRGB,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        texture.view                    = _device->createImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB);
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        VK_CHECK_RESULT(vkCreateSampler(_device->logicalDevice, &samplerInfo, nullptr, &texture.sampler));
        texture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        _textures.push_back(texture);

        stagingBuffer.destroy();
    }
}
void VulkanRenderable::loadResouces(VkQueue queue) {
    // Create and upload vertex and index buffer
    size_t vertexBufferSize = entity->vertices.size() * sizeof(vkl::VertexLayout);
    size_t indexBufferSize  = entity->indices.size() * sizeof(entity->indices[0]);

    loadImages(queue);
    _mesh.setup(_device, queue, entity->vertices, entity->indices, vertexBufferSize, indexBufferSize);
}
void VulkanRenderable::drawNode(const Entity::Node *node) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix    = node->matrix;
        Entity::Node     *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        vkCmdPushConstants(drawCmd, shaderPass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                           &nodeMatrix);
        for (const Entity::Primitive primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderPass->layout, 1, 1,
                                        &materialSets[primitive.materialIndex], 0, nullptr);
                vkCmdDrawIndexed(drawCmd, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (Entity::Node *child : node->children) {
        drawNode(child);
    }
}
void VulkanRenderable::draw() {
    assert(shaderPass);
    VkDeviceSize offsets[1] = {0};
    vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderPass->layout, 0, 1, &globalDescriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(drawCmd, 0, 1, &_mesh.vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(drawCmd, _mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderPass->builtPipeline);

    // manual created
    if (entity->_nodes.empty()) {
        if (!_textures.empty()) {
            vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderPass->layout, 1, 1, materialSets.data(), 0, nullptr);
        }

        vkCmdBindPipeline(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderPass->builtPipeline);
        vkCmdDrawIndexed(drawCmd, _mesh.getIndicesCount(), 1, 0, 0, 0);
    }
    // file loaded
    else {
        for (Entity::Node *node : entity->_nodes) {
            node->matrix = transform;
            drawNode(node);
        }
    }
}
void VulkanSceneRenderer::drawScene() {
    for (auto &renderable : _renderList) {
        renderable->draw();
    }
}
VulkanRenderable::VulkanRenderable(SceneRenderer *renderer, vkl::Device *device, vkl::Entity *entity, const VkCommandBuffer drawCmd)
    : Renderable(renderer, entity), _device(device), drawCmd(drawCmd) {
}
} // namespace vkl
