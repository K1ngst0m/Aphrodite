#include "renderable.h"

namespace vkl {
VulkanRenderable::VulkanRenderable(SceneRenderer *renderer, vkl::Device *device, vkl::Entity *entity, const VkCommandBuffer drawCmd)
    : Renderable(renderer, entity), _device(device), drawCmd(drawCmd) {
    _shaderPass = entity->_pass;
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
    size_t vertexBufferSize = entity->vertices.size() * sizeof(entity->vertices[0]);
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
        vkCmdPushConstants(drawCmd, _shaderPass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                           &nodeMatrix);
        for (const Entity::Primitive primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 1, 1,
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
    assert(_shaderPass);
    VkDeviceSize offsets[1] = {0};
    vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 0, 1, &globalDescriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(drawCmd, 0, 1, &_mesh.vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(drawCmd, _mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->builtPipeline);

    // manual created
    if (entity->_nodes.empty()) {
        if (!_textures.empty()) {
            vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 1, 1, materialSets.data(), 0, nullptr);
        }

        vkCmdBindPipeline(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->builtPipeline);
        vkCmdDrawIndexed(drawCmd, _mesh.getIndicesCount(), 1, 0, 0, 0);
    }
    // file loaded
    else {
        for (Entity::Node *node : entity->_nodes) {
            drawNode(node);
        }
    }
}
void VulkanRenderable::cleanupResources() {
    _mesh.destroy();
    for (auto & texture : _textures){
        texture.destroy();
    }
}
} // namespace vkl
