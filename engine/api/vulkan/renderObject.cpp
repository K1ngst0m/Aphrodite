#include "renderObject.h"

namespace vkl {
VulkanRenderObject::VulkanRenderObject(SceneRenderer *renderer, vkl::VulkanDevice *device, vkl::Entity *entity, const VkCommandBuffer drawCmd)
    : _device(device), _drawCmd(drawCmd), _renderer(renderer), _entity(entity) {
}
void VulkanRenderObject::setupMaterialDescriptor(VkDescriptorSetLayout layout, VkDescriptorPool descriptorPool) {
    for (auto &material : _entity->_materials) {
        MaterialGpuData materialData {};
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(descriptorPool, &layout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->logicalDevice, &allocInfo, &materialData.set));
        std::vector<VkWriteDescriptorSet> descriptorWrites{
            vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &_textures[material.baseColorTextureIndex].descriptorInfo),
            vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_textures[material.normalTextureIndex].descriptorInfo),
        };
        vkUpdateDescriptorSets(_device->logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        materialData.pipeline = _shaderPass->builtPipeline;

        _materialGpuDataList.push_back(materialData);
    }
}
void VulkanRenderObject::loadImages(VkQueue queue) {
    for (auto &image : _entity->_images) {
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

    loadImages(queue);
    loadBuffer(_device, queue, _entity->_vertices, _entity->_indices, vertexBufferSize, indexBufferSize);
}
void VulkanRenderObject::drawNode(const SubEntity *node) {
    if (!node->primitives.empty()) {
        glm::mat4  nodeMatrix    = node->matrix;
        SubEntity *currentParent = node->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        vkCmdPushConstants(_drawCmd, _shaderPass->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
        for (const Primitive primitive : node->primitives) {
            if (primitive.indexCount > 0) {
                vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 1, 1,
                                        &_materialGpuDataList[primitive.materialIndex].set, 0, nullptr);
                vkCmdDrawIndexed(_drawCmd, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }
    for (const auto &child : node->children) {
        drawNode(child.get());
    }
}
void VulkanRenderObject::draw(VkDescriptorSet* globalSet) {
    assert(_shaderPass);
    VkDeviceSize offsets[1] = {0};
    vkCmdBindDescriptorSets(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->layout, 0, 1, globalSet, 0, nullptr);
    vkCmdBindVertexBuffers(_drawCmd, 0, 1, &_vertexBuffer.buffer.buffer, offsets);
    vkCmdBindIndexBuffer(_drawCmd, _indexBuffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(_drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _shaderPass->builtPipeline);

    for (auto &node : _entity->_subEntityList) {
        drawNode(node.get());
    }
}
void VulkanRenderObject::cleanupResources() {
    _vertexBuffer.buffer.destroy();
    _indexBuffer.buffer.destroy();
    for (auto &texture : _textures) {
        texture.destroy();
    }
}
void VulkanRenderObject::setShaderPass(ShaderPass *pass) {
    _shaderPass = pass;
}
ShaderPass *VulkanRenderObject::getShaderPass() const {
    return _shaderPass;
}
uint32_t VulkanRenderObject::getSetCount() {
    return 1 + _entity->_materials.size();
}

void VulkanRenderObject::loadBuffer(vkl::VulkanDevice *device, VkQueue transferQueue, std::vector<VertexLayout> vertices, std::vector<uint32_t> indices, uint32_t vSize, uint32_t iSize) {
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
        VkDeviceSize bufferSize = vSize == 0 ? sizeof(_vertexBuffer.vertices[0]) * _vertexBuffer.vertices.size() : vSize;
        // using staging buffer
        if (transferQueue != VK_NULL_HANDLE) {
            vkl::VulkanBuffer stagingBuffer;
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(_vertexBuffer.vertices.data(), static_cast<VkDeviceSize>(bufferSize));
            stagingBuffer.unmap();

            device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer.buffer);
            device->copyBuffer(transferQueue, stagingBuffer, _vertexBuffer.buffer, bufferSize);

            stagingBuffer.destroy();
        }

        else {
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _vertexBuffer.buffer);
            _vertexBuffer.buffer.map();
            _vertexBuffer.buffer.copyTo(_vertexBuffer.vertices.data(), static_cast<VkDeviceSize>(bufferSize));
            _vertexBuffer.buffer.unmap();
        }
    }

    // setup index buffer
    {
        VkDeviceSize bufferSize = iSize == 0 ? sizeof(_indexBuffer.indices[0]) * _indexBuffer.indices.size() : iSize;
        // using staging buffer
        if (transferQueue != VK_NULL_HANDLE) {
            vkl::VulkanBuffer stagingBuffer;
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

            stagingBuffer.map();
            stagingBuffer.copyTo(_indexBuffer.indices.data(), static_cast<VkDeviceSize>(bufferSize));
            stagingBuffer.unmap();

            device->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer.buffer);
            device->copyBuffer(transferQueue, stagingBuffer, _indexBuffer.buffer, bufferSize);

            stagingBuffer.destroy();
        }

        else {
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
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
} // namespace vkl
