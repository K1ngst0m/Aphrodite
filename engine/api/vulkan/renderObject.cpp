#include "renderObject.h"

namespace vkl {
VulkanRenderObject::VulkanRenderObject(SceneRenderer *renderer, vkl::Device *device, vkl::Entity *entity, const VkCommandBuffer drawCmd)
    : RenderObject(renderer, entity), _device(device), _drawCmd(drawCmd) {
}
vkl::Texture *VulkanRenderObject::getTexture(uint32_t index) {
    if (index < _textures.size()) {
        return &_textures[index];
    }
    return nullptr;
}
void VulkanRenderObject::setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) {
    for (auto &material : _entity->_materials) {
        VkDescriptorSet             materialSet;
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(descriptorPool, &layout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &materialSet));
        std::vector<VkWriteDescriptorSet> descriptorWrites{
            vkl::init::writeDescriptorSet(materialSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &getTexture(material.baseColorTextureIndex)->descriptorInfo),
            vkl::init::writeDescriptorSet(materialSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &getTexture(material.normalTextureIndex)->descriptorInfo),
        };
        vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        _materialSets.push_back(materialSet);
    }
}
void VulkanRenderObject::loadImages(VkQueue queue) {
    for (auto &image : _entity->_images) {
        unsigned char *imageData     = image.data.data();
        uint32_t       imageDataSize = image.data.size();
        uint32_t       width         = image.width;
        uint32_t       height        = image.height;

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
void VulkanRenderObject::loadResouces(VkQueue queue) {
    // Create and upload vertex and index buffer
    size_t vertexBufferSize = _entity->_vertices.size() * sizeof(_entity->_vertices[0]);
    size_t indexBufferSize  = _entity->_indices.size() * sizeof(_entity->_indices[0]);

    loadImages(queue);
    _mesh.setup(_device, queue, _entity->_vertices, _entity->_indices, vertexBufferSize, indexBufferSize);
}
void VulkanRenderObject::drawNode(const SubEntity *node) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix    = node->matrix;
        SubEntity     *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        vkCmdPushConstants(_drawCmd, _shaderPass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                           &nodeMatrix);
        for (const Primitive primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 1, 1,
                                        &_materialSets[primitive.materialIndex], 0, nullptr);
                vkCmdDrawIndexed(_drawCmd, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (SubEntity *child : node->children) {
        drawNode(child);
    }
}
void VulkanRenderObject::draw() {
    assert(_shaderPass);
    VkDeviceSize offsets[1] = {0};
    vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 0, 1, &_globalDescriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(_drawCmd, 0, 1, &_mesh.vertexBuffer.buffer, offsets);
    vkCmdBindIndexBuffer(_drawCmd, _mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->builtPipeline);

    for (SubEntity *node : _entity->_subEntityList) {
        drawNode(node);
    }
}
void VulkanRenderObject::cleanupResources() {
    _mesh.destroy();
    for (auto & texture : _textures){
        texture.destroy();
    }
}
void VulkanRenderObject::setShaderPass(ShaderPass *pass) {
    _shaderPass = pass;
}
ShaderPass *VulkanRenderObject::getShaderPass() const {
    return _shaderPass;
}
VkDescriptorSet &VulkanRenderObject::getGlobalDescriptorSet() {
    return _globalDescriptorSet;
}
uint32_t VulkanRenderObject::getSetCount() {
    return 1 + _entity->_materials.size();
}
} // namespace vkl
