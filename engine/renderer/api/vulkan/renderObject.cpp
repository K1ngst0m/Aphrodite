#include "renderObject.h"
#include "device.h"
#include "pipeline.h"
#include "scene/entity.h"
#include "sceneRenderer.h"
#include "vkInit.hpp"

namespace vkl {

VulkanRenderObject::VulkanRenderObject(VulkanSceneRenderer *renderer, const std::shared_ptr<VulkanDevice> & device, vkl::Entity *entity)
    : _device(device), _renderer(renderer), _entity(entity) {
}
void VulkanRenderObject::setupMaterial(VkDescriptorSetLayout* materialLayout, VkDescriptorPool descriptorPool, uint8_t bindingBits) {
    for (auto &material : _entity->_materials) {
        MaterialGpuData             materialData{};
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(descriptorPool, materialLayout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &materialData.set));
        std::vector<VkWriteDescriptorSet> descriptorWrites{};

        if (bindingBits & MATERIAL_BINDING_BASECOLOR){
            if (material.baseColorTextureIndex > -1) {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &_textures[material.baseColorTextureIndex].descriptorInfo));
            } else {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &_emptyTexture.descriptorInfo));
                std::cerr << "base color texture not found, use default texture." << std::endl;
            }
        }

        if (bindingBits & MATERIAL_BINDING_NORMAL){
            if (material.normalTextureIndex > -1) {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_textures[material.normalTextureIndex].descriptorInfo));
            } else {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_emptyTexture.descriptorInfo));
                std::cerr << "material id: [" << material.id << "] :";
                std::cerr << "normal texture not found, use default texture." << std::endl;
            }
        }

        vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        _materialGpuDataList.push_back(materialData);
    }
}
void VulkanRenderObject::loadTextures(VkQueue queue) {
    for (auto &image : _entity->_textures) {
        unsigned char *imageData     = image.data.data();
        uint32_t       imageDataSize = image.data.size();
        uint32_t       width         = image.width;
        uint32_t       height        = image.height;

        // Load texture from image buffer
        vkl::VulkanBuffer stagingBuffer;
        _device->createBuffer(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

        stagingBuffer.map();
        stagingBuffer.copyTo(imageData, static_cast<size_t>(imageDataSize));
        stagingBuffer.unmap();

        vkl::VulkanTexture texture;
        _device->createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture);

        _device->transitionImageLayout(queue, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        _device->copyBufferToImage(queue, stagingBuffer.buffer, texture.image, width, height);
        _device->transitionImageLayout(queue, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        texture.view = _device->createImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB);

        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.maxAnisotropy       = _device->enabledFeatures.samplerAnisotropy ? _device->properties.limits.maxSamplerAnisotropy : 1.0f;
        samplerInfo.anisotropyEnable    = _device->enabledFeatures.samplerAnisotropy;
        samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

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

    createEmptyTexture(queue);
    loadTextures(queue);
    loadBuffer(queue);
}
void VulkanRenderObject::drawNode(VkPipelineLayout layout, VkCommandBuffer drawCmd, const std::shared_ptr<SubEntity>& node) {
    if (!node->isVisible) {
        return;
    }
    if (!node->primitives.empty()) {
        glm::mat4  nodeMatrix    = node->matrix;
        SubEntity *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        nodeMatrix = _transform * nodeMatrix;
        vkCmdPushConstants(drawCmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
        for (const Primitive primitive : node->primitives) {
            if (primitive.indexCount > 0) {
                MaterialGpuData &materialData = _materialGpuDataList[primitive.materialIndex];
                vkCmdBindDescriptorSets(drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &materialData.set, 0, nullptr);
                vkCmdDrawIndexed(drawCmd, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }

    for (const auto &child : node->children) {
        drawNode(layout, drawCmd, child);
    }
}
void VulkanRenderObject::draw(VkPipelineLayout layout, VkCommandBuffer drawCmd) {
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(drawCmd, 0, 1, &_vertexBuffer.buffer.buffer, offsets);
    vkCmdBindIndexBuffer(drawCmd, _indexBuffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    for (auto &subEntity : _entity->_subEntityList) {
        drawNode(layout, drawCmd, subEntity);
    }
}
void VulkanRenderObject::cleanupResources() {
    _vertexBuffer.buffer.destroy();
    _indexBuffer.buffer.destroy();
    for (auto &texture : _textures) {
        texture.destroy();
    }
    _emptyTexture.destroy();
}
uint32_t VulkanRenderObject::getSetCount() {
    return _entity->_materials.size();
}
void VulkanRenderObject::loadBuffer(VkQueue transferQueue) {
    auto& vertices = _entity->_vertices;
    auto& indices  = _entity->_indices;

    if (!vertices.empty()) {
        _vertexBuffer.vertices = std::move(vertices);
    }
    if (!indices.empty()) {
        _indexBuffer.indices = std::move(indices);
    }

    assert(!_vertexBuffer.vertices.empty());

    if (_indexBuffer.indices.empty()) {
        for (size_t i = 0; i < _vertexBuffer.vertices.size(); i++) {
            _indexBuffer.indices.push_back(i);
        }
    }

    // setup vertex buffer
    {
        auto         vSize      = vertices.size();
        VkDeviceSize bufferSize = vSize == 0 ? sizeof(_vertexBuffer.vertices[0]) * _vertexBuffer.vertices.size() : vSize;
        // using staging buffer
        if (transferQueue != VK_NULL_HANDLE) {
            vkl::VulkanBuffer stagingBuffer;
            _device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(_vertexBuffer.vertices.data(), static_cast<VkDeviceSize>(bufferSize));
            stagingBuffer.unmap();

            _device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer.buffer);
            _device->copyBuffer(transferQueue, stagingBuffer, _vertexBuffer.buffer, bufferSize);

            stagingBuffer.destroy();
        }

        else {
            _device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _vertexBuffer.buffer);
            _vertexBuffer.buffer.map();
            _vertexBuffer.buffer.copyTo(_vertexBuffer.vertices.data(), static_cast<VkDeviceSize>(bufferSize));
            _vertexBuffer.buffer.unmap();
        }
    }

    // setup index buffer
    {
        auto         iSize      = indices.size();
        VkDeviceSize bufferSize = iSize == 0 ? sizeof(_indexBuffer.indices[0]) * _indexBuffer.indices.size() : iSize;
        // using staging buffer
        if (transferQueue != VK_NULL_HANDLE) {
            vkl::VulkanBuffer stagingBuffer;
            _device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(_indexBuffer.indices.data(), static_cast<VkDeviceSize>(bufferSize));
            stagingBuffer.unmap();

            _device->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer.buffer);
            _device->copyBuffer(transferQueue, stagingBuffer, _indexBuffer.buffer, bufferSize);

            stagingBuffer.destroy();
        }

        else {
            _device->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _indexBuffer.buffer);
            _indexBuffer.buffer.map();
            _indexBuffer.buffer.copyTo(_indexBuffer.indices.data(), static_cast<VkDeviceSize>(bufferSize));
            _indexBuffer.buffer.unmap();
        }
    }
}
glm::mat4 VulkanRenderObject::getTransform() const {
    return _transform;
}
void VulkanRenderObject::setTransform(glm::mat4 transform) {
    _transform = transform;
}
void VulkanRenderObject::createEmptyTexture(VkQueue queue) {
    uint32_t width         = 1;
    uint32_t height        = 1;
    uint32_t imageDataSize = width * height * 4;

    // Load texture from image buffer
    vkl::VulkanBuffer stagingBuffer;
    _device->createBuffer(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    uint8_t *data;
    stagingBuffer.map();
    stagingBuffer.copyTo(data, imageDataSize);
    stagingBuffer.unmap();

    vkl::VulkanTexture &texture = _emptyTexture;
    _device->createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture);

    _device->transitionImageLayout(queue, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    _device->copyBufferToImage(queue, stagingBuffer.buffer, texture.image, width, height);
    _device->transitionImageLayout(queue, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    texture.view = _device->createImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB);

    VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
    samplerInfo.maxAnisotropy       = _device->enabledFeatures.samplerAnisotropy ? _device->properties.limits.maxSamplerAnisotropy : 1.0f;
    samplerInfo.anisotropyEnable    = _device->enabledFeatures.samplerAnisotropy;
    samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VK_CHECK_RESULT(vkCreateSampler(_device->logicalDevice, &samplerInfo, nullptr, &texture.sampler));
    texture.setupDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.destroy();
}
} // namespace vkl
