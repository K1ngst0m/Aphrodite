#include "renderObject.h"
#include "buffer.h"
#include "device.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "sampler.h"
#include "scene/entity.h"
#include "sceneRenderer.h"
#include "vkInit.hpp"

namespace vkl {

VulkanRenderObject::VulkanRenderObject(VulkanSceneRenderer *renderer, const std::shared_ptr<VulkanDevice> &device, vkl::Entity *entity)
    : _device(device), _renderer(renderer), _entity(entity) {
}

void VulkanRenderObject::setupMaterial(VkDescriptorSetLayout *materialLayout, VkDescriptorPool descriptorPool, uint8_t bindingBits) {
    for (auto &material : _entity->_materials) {
        MaterialGpuData             materialData{};
        VkDescriptorSetAllocateInfo allocInfo = vkl::init::descriptorSetAllocateInfo(descriptorPool, materialLayout, 1);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(_device->getLogicalDevice(), &allocInfo, &materialData.set));
        std::vector<VkWriteDescriptorSet> descriptorWrites{};

        if (bindingBits & MATERIAL_BINDING_BASECOLOR) {
            if (material.baseColorTextureIndex > -1) {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &_textures[material.baseColorTextureIndex].descriptorInfo));
            } else {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &_emptyTexture.descriptorInfo));
                std::cerr << "base color texture not found, use default texture." << std::endl;
            }
        }

        if (bindingBits & MATERIAL_BINDING_NORMAL) {
            if (material.normalTextureIndex > -1) {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_textures[material.normalTextureIndex].descriptorInfo));
            } else {
                descriptorWrites.push_back(vkl::init::writeDescriptorSet(materialData.set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_emptyTexture.descriptorInfo));
                std::cerr << "material id: [" << material.id << "] :";
                std::cerr << "normal texture not found, use default texture." << std::endl;
            }
        }

        vkUpdateDescriptorSets(_device->getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        _materialGpuDataList.push_back(materialData);
    }
}
void VulkanRenderObject::loadTextures() {
    for (auto &image : _entity->_textures) {
        // raw image data
        unsigned char *imageData     = image.data.data();
        uint32_t       imageDataSize = image.data.size();
        uint32_t       width         = image.width;
        uint32_t       height        = image.height;

        // staging buffer
        VulkanBuffer *stagingBuffer;
        {
            BufferCreateInfo createInfo{};
            createInfo.size     = imageDataSize;
            createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            _device->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(imageData, static_cast<size_t>(imageDataSize));
        stagingBuffer->unmap();

        // texture
        TextureGpuData texture;
        texture.image     = new VulkanImage;
        texture.imageView = new VulkanImageView;
        // texture.sampler   = new VulkanSampler;

        // texture image resource
        {
            ImageCreateInfo createInfo{};
            createInfo.extent   = {width, height, 1};
            createInfo.format   = FORMAT_R8G8B8A8_SRGB;
            createInfo.tiling   = IMAGE_TILING_OPTIMAL;
            createInfo.usage    = IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT;
            createInfo.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            _device->createImage(&createInfo, &texture.image);

            _device->immediateSubmit(QUEUE_TYPE_TRANSFER, [&](VkCommandBuffer cmd) {
                _device->transitionImageLayout(cmd, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                _device->copyBufferToImage(cmd, stagingBuffer, texture.image);
                _device->transitionImageLayout(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            });
        }

        // texture image view
        {
            ImageViewCreateInfo createInfo{};
            createInfo.format   = FORMAT_R8G8B8A8_SRGB;
            createInfo.viewType = IMAGE_VIEW_TYPE_2D;
            _device->createImageView(&createInfo, &texture.imageView, texture.image);
        }

        // texture sampler
        {
            VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
            samplerInfo.maxAnisotropy       = _device->getDeviceEnabledFeatures().samplerAnisotropy ? _device->getDeviceProperties().limits.maxSamplerAnisotropy : 1.0f;
            samplerInfo.anisotropyEnable    = _device->getDeviceEnabledFeatures().samplerAnisotropy;
            samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            VK_CHECK_RESULT(vkCreateSampler(_device->getLogicalDevice(), &samplerInfo, nullptr, &texture.sampler));
        }

        texture.descriptorInfo = vkl::init::descriptorImageInfo(texture.sampler, texture.imageView->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        _textures.push_back(texture);

        _device->destroyBuffer(stagingBuffer);
    }
}
void VulkanRenderObject::loadResouces() {
    // Create and upload vertex and index buffer
    size_t vertexBufferSize = _entity->_vertices.size() * sizeof(_entity->_vertices[0]);
    size_t indexBufferSize  = _entity->_indices.size() * sizeof(_entity->_indices[0]);

    createEmptyTexture();
    loadTextures();
    loadBuffer();
}
void VulkanRenderObject::drawNode(VkPipelineLayout layout, VkCommandBuffer drawCmd, const std::shared_ptr<SubEntity> &node) {
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
    vkCmdBindVertexBuffers(drawCmd, 0, 1, &_vertexBuffer.buffer->getHandle(), offsets);
    vkCmdBindIndexBuffer(drawCmd, _indexBuffer.buffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);
    for (auto &subEntity : _entity->_subEntityList) {
        drawNode(layout, drawCmd, subEntity);
    }
}
void VulkanRenderObject::cleanupResources() {
    _device->destroyBuffer(_vertexBuffer.buffer);
    _device->destroyBuffer(_indexBuffer.buffer);

    for (TextureGpuData &texture : _textures) {
        _device->destroyImage(texture.image);
        _device->destroyImageView(texture.imageView);
        vkDestroySampler(_device->getLogicalDevice(), texture.sampler, nullptr);
    }

    _device->destroyImage(_emptyTexture.image);
    _device->destroyImageView(_emptyTexture.imageView);
    vkDestroySampler(_device->getLogicalDevice(), _emptyTexture.sampler, nullptr);
}
uint32_t VulkanRenderObject::getSetCount() {
    return _entity->_materials.size();
}
void VulkanRenderObject::loadBuffer() {
    auto &vertices = _entity->_vertices;
    auto &indices  = _entity->_indices;

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
        vkl::VulkanBuffer *stagingBuffer;
        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
            _device->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(_vertexBuffer.vertices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            _device->createBuffer(&createInfo, &_vertexBuffer.buffer);
        }

        _device->immediateSubmit(QUEUE_TYPE_TRANSFER, [&](VkCommandBuffer cmd) {
            _device->copyBuffer(cmd, stagingBuffer, _vertexBuffer.buffer, bufferSize);
        });

        _device->destroyBuffer(stagingBuffer);
    }

    // setup index buffer
    {
        auto         iSize      = indices.size();
        VkDeviceSize bufferSize = iSize == 0 ? sizeof(_indexBuffer.indices[0]) * _indexBuffer.indices.size() : iSize;
        // using staging buffer
        vkl::VulkanBuffer *stagingBuffer;

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
            _device->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(_indexBuffer.indices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage    = BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            _device->createBuffer(&createInfo, &_indexBuffer.buffer);
        }

        _device->immediateSubmit(QUEUE_TYPE_TRANSFER, [&](VkCommandBuffer cmd) {
            _device->copyBuffer(cmd, stagingBuffer, _indexBuffer.buffer, bufferSize);
        });

        _device->destroyBuffer(stagingBuffer);
    }
}
glm::mat4 VulkanRenderObject::getTransform() const {
    return _transform;
}
void VulkanRenderObject::setTransform(glm::mat4 transform) {
    _transform = transform;
}
void VulkanRenderObject::createEmptyTexture() {
    uint32_t width         = 1;
    uint32_t height        = 1;
    uint32_t imageDataSize = width * height * 4;

    // Load texture from image buffer
    vkl::VulkanBuffer *stagingBuffer;
    BufferCreateInfo   createInfo{};
    createInfo.size     = imageDataSize;
    createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
    _device->createBuffer(&createInfo, &stagingBuffer);

    uint8_t data{};
    stagingBuffer->map();
    stagingBuffer->copyTo(&data, imageDataSize);
    stagingBuffer->unmap();

    {
        ImageCreateInfo createInfo{};
        createInfo.extent   = {width, height, 1};
        createInfo.format   = FORMAT_R8G8B8A8_SRGB;
        createInfo.tiling   = IMAGE_TILING_OPTIMAL;
        createInfo.usage    = IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT;
        createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        _device->createImage(&createInfo, &_emptyTexture.image);

        _device->immediateSubmit(QUEUE_TYPE_TRANSFER, [&](VkCommandBuffer cmd) {
            _device->transitionImageLayout(cmd, _emptyTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            _device->copyBufferToImage(cmd, stagingBuffer, _emptyTexture.image);
            _device->transitionImageLayout(cmd, _emptyTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
    }

    {
        ImageViewCreateInfo createInfo{};
        createInfo.format   = FORMAT_R8G8B8A8_SRGB;
        createInfo.viewType = IMAGE_VIEW_TYPE_2D;
        VK_CHECK_RESULT(_device->createImageView(&createInfo, &_emptyTexture.imageView, _emptyTexture.image));
    }

    {
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.maxAnisotropy       = _device->getDeviceEnabledFeatures().samplerAnisotropy ? _device->getDeviceProperties().limits.maxSamplerAnisotropy : 1.0f;
        samplerInfo.anisotropyEnable    = _device->getDeviceEnabledFeatures().samplerAnisotropy;
        samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(vkCreateSampler(_device->getLogicalDevice(), &samplerInfo, nullptr, &_emptyTexture.sampler));
    }

    _emptyTexture.descriptorInfo = vkl::init::descriptorImageInfo(_emptyTexture.sampler, _emptyTexture.imageView->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _device->destroyBuffer(stagingBuffer);
}
} // namespace vkl
