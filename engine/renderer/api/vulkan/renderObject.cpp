#include "renderObject.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "descriptorSetLayout.h"
#include "device.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "sampler.h"
#include "scene/entity.h"
#include "sceneRenderer.h"
#include "vkInit.hpp"

namespace vkl {
struct ObjectInfo{
    glm::mat4 matrix = glm::mat4(1.0f);
};

struct MaterialInfo{
    glm::vec4 emissiveFactor = glm::vec4(1.0f);
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float     alphaCutoff     = 1.0f;
    float     metallicFactor  = 1.0f;
    float     roughnessFactor = 1.0f;
    ResourceIndex baseColorTextureIndex = -1;
    ResourceIndex normalTextureIndex    = -1;
    ResourceIndex occlusionTextureIndex = -1;
    ResourceIndex emissiveTextureIndex  = -1;
    ResourceIndex metallicRoughnessTextureIndex = -1;
    ResourceIndex specularGlossinessTextureIndex = -1;
};

VulkanRenderData::VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode)
    : _device(device), _node(std::move(sceneNode)) {
}

void VulkanRenderData::setupDescriptor(VulkanDescriptorSetLayout * objectLayout, VulkanDescriptorSetLayout *materialLayout, uint8_t bindingBits) {
    {
        ObjectInfo objInfo{};
        BufferCreateInfo bufferCI{
            .size = sizeof(ObjectInfo),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        VK_CHECK_RESULT(_device->createBuffer(&bufferCI, &_objectUB, &objInfo));
        _objectUB->setupDescriptor();
    }

    {
        _objectSet = objectLayout->allocateSet();

        std::vector<VkWriteDescriptorSet> descriptorWrites{
            vkl::init::writeDescriptorSet(_objectSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &_objectUB->getBufferInfo())
        };

        vkUpdateDescriptorSets(_device->getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    for (auto &material : _node->getObject<Entity>()->_materials) {
        // write descriptor set
        auto set = materialLayout->allocateSet();
        {
            VulkanBuffer * matInfoUB = nullptr;
            {
                MaterialInfo matInfo{
                    .emissiveFactor = material.emissiveFactor,
                    .baseColorFactor = material.baseColorFactor,
                    .alphaCutoff = material.alphaCutoff,
                    .metallicFactor = material.metallicFactor,
                    .roughnessFactor = material.roughnessFactor,
                    .baseColorTextureIndex = material.baseColorTextureIndex,
                    .normalTextureIndex = material.normalTextureIndex,
                    .occlusionTextureIndex = material.occlusionTextureIndex,
                    .emissiveTextureIndex = material.emissiveTextureIndex,
                    .specularGlossinessTextureIndex = material.specularGlossinessTextureIndex,
                };
                BufferCreateInfo bufferCI{
                    .size = sizeof(MaterialInfo),
                    .alignment = 0,
                    .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };
                _device->createBuffer(&bufferCI, &matInfoUB, &matInfo);
                matInfoUB->setupDescriptor();
            }
            std::vector<VkWriteDescriptorSet> descriptorWrites{};
            // create material buffer Info
            descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matInfoUB->getBufferInfo()));

            if (bindingBits & MATERIAL_BINDING_BASECOLOR) {
                if (material.baseColorTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_textures[material.baseColorTextureIndex].descriptorInfo));
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &_emptyTexture.descriptorInfo));
                    std::cerr << "base color texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_NORMAL) {
                if (material.normalTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &_textures[material.normalTextureIndex].descriptorInfo));
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &_emptyTexture.descriptorInfo));
                    std::cerr << "material id: [" << material.id << "] :";
                    std::cerr << "normal texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_PHYSICAL){
                if (material.metallicFactor > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &_textures[material.metallicRoughnessTextureIndex].descriptorInfo));
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &_emptyTexture.descriptorInfo));
                    std::cerr << "material id: [" << material.id << "] :";
                    std::cerr << "physical desc texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_AO){
                if (material.occlusionTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &_textures[material.occlusionTextureIndex].descriptorInfo));
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &_emptyTexture.descriptorInfo));
                    std::cerr << "material id: [" << material.id << "] :";
                    std::cerr << "ao texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_EMISSIVE){
                if (material.emissiveTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &_textures[material.emissiveTextureIndex].descriptorInfo));
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &_emptyTexture.descriptorInfo));
                    std::cerr << "material id: [" << material.id << "] :";
                    std::cerr << "emissive texture not found, use default texture." << std::endl;
                }
            }
            vkUpdateDescriptorSets(_device->getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        _materialSets.push_back(set);
    }
}

void VulkanRenderData::loadResouces() {
    loadTextures();
    loadBuffer();
}

void VulkanRenderData::loadTextures() {
    // create empty texture
    {
        uint32_t width         = 1;
        uint32_t height        = 1;
        uint32_t imageDataSize = width * height * 4;

        uint8_t data{0};
        _emptyTexture = createTexture(width, height, &data, imageDataSize);
    }

    for (auto &image : _node->getObject<Entity>()->_images) {
        // raw image data
        unsigned char *imageData     = image.data.data();
        uint32_t       imageDataSize = image.data.size();
        uint32_t       width         = image.width;
        uint32_t       height        = image.height;

        auto texture = createTexture(width, height, imageData, imageDataSize);
        _textures.push_back(texture);
    }
}

void VulkanRenderData::cleanupResources() {
    _device->destroyBuffer(_meshData.vb);
    _device->destroyBuffer(_meshData.ib);

    for (TextureGpuData &texture : _textures) {
        _device->destroyImage(texture.image);
        _device->destroyImageView(texture.imageView);
        vkDestroySampler(_device->getHandle(), texture.sampler, nullptr);
    }

    _device->destroyImage(_emptyTexture.image);
    _device->destroyImageView(_emptyTexture.imageView);
    vkDestroySampler(_device->getHandle(), _emptyTexture.sampler, nullptr);
}

void VulkanRenderData::draw(VulkanPipeline * pipeline, VulkanCommandBuffer *drawCmd) {
    VkDeviceSize offsets[1] = {0};
    drawCmd->cmdBindVertexBuffers(0, 1, _meshData.vb, offsets);
    drawCmd->cmdBindIndexBuffers(_meshData.ib, 0, VK_INDEX_TYPE_UINT32);

    std::queue<std::shared_ptr<Node>> q;
    for (auto &node : _node->getObject<Entity>()->_subNodeList) {
        if (node->isVisible){
            q.push(node);
        }
    }

    while(!q.empty()){
        auto subNode = q.front();
        q.pop();

        glm::mat4 nodeMatrix    = subNode->matrix;
        Node     *currentParent = subNode->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        nodeMatrix = _node->getTransform() * nodeMatrix;
        drawCmd->cmdPushConstants(pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);

        for (const auto& subset : subNode->subsets) {
            if (subset.indexCount > 0) {
                auto &materialSet = _materialSets[subset.materialIndex];
                drawCmd->cmdBindDescriptorSet(pipeline, 2, 1, &materialSet);
                drawCmd->cmdDrawIndexed(subset.indexCount, 1, subset.firstIndex, 0, 0);
            }
        }

        for (const auto &child : subNode->children){
            q.push(child);
        }
    }
}

uint32_t VulkanRenderData::getSetCount() {
    return _node->getObject<Entity>()->_materials.size();
}

void VulkanRenderData::loadBuffer() {
    auto &vertices = _node->getObject<Entity>()->_vertices;
    auto &indices  = _node->getObject<Entity>()->_indices;

    assert(!vertices.empty());

    if (indices.empty()) {
        for (size_t i = 0; i < vertices.size(); i++) {
            indices.push_back(i);
        }
    }

    // setup vertex buffer
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
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
        stagingBuffer->copyTo(vertices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            _device->createBuffer(&createInfo, &_meshData.vb);
        }

        auto cmd = _device->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, _meshData.vb, bufferSize);
        _device->endSingleTimeCommands(cmd);

        _device->destroyBuffer(stagingBuffer);
    }

    // setup index buffer
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        // using staging buffer
        vkl::VulkanBuffer *stagingBuffer = nullptr;

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
            createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
            _device->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(indices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage    = BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            _device->createBuffer(&createInfo, &_meshData.ib);
        }

        auto cmd = _device->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, _meshData.ib, bufferSize);
        _device->endSingleTimeCommands(cmd);

        _device->destroyBuffer(stagingBuffer);
    }
}

TextureGpuData VulkanRenderData::createTexture(uint32_t width, uint32_t height, void *data, uint32_t dataSize) {
    uint32_t texMipLevels = calculateFullMipLevels(width, height);

    // Load texture from image buffer
    vkl::VulkanBuffer *stagingBuffer;
    {
        BufferCreateInfo createInfo{};
        createInfo.size     = dataSize;
        createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
        _device->createBuffer(&createInfo, &stagingBuffer);

        stagingBuffer->map();
        stagingBuffer->copyTo(data, dataSize);
        stagingBuffer->unmap();
    }

    TextureGpuData texture{};
    {
        ImageCreateInfo createInfo{};
        createInfo.extent    = {width, height, 1};
        createInfo.format    = FORMAT_R8G8B8A8_SRGB;
        createInfo.tiling    = IMAGE_TILING_OPTIMAL;
        createInfo.usage     = IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT; createInfo.property  = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        createInfo.mipLevels = texMipLevels;

        _device->createImage(&createInfo, &texture.image);

        VulkanCommandBuffer *cmd = _device->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cmd->cmdCopyBufferToImage(stagingBuffer, texture.image);
        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        _device->endSingleTimeCommands(cmd);

        cmd = _device->beginSingleTimeCommands(VK_QUEUE_GRAPHICS_BIT);

        // generate mipmap chains
        for (int32_t i = 1; i < texMipLevels; i++) {
            VkImageBlit imageBlit{};

            // Source
            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.layerCount = 1;
            imageBlit.srcSubresource.mipLevel   = i - 1;
            imageBlit.srcOffsets[1].x           = int32_t(width >> (i - 1));
            imageBlit.srcOffsets[1].y           = int32_t(height >> (i - 1));
            imageBlit.srcOffsets[1].z           = 1;

            // Destination
            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.layerCount = 1;
            imageBlit.dstSubresource.mipLevel   = i;
            imageBlit.dstOffsets[1].x           = int32_t(width >> i);
            imageBlit.dstOffsets[1].y           = int32_t(height >> i);
            imageBlit.dstOffsets[1].z           = 1;

            VkImageSubresourceRange mipSubRange = {};
            mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
            mipSubRange.baseMipLevel            = i;
            mipSubRange.levelCount              = 1;
            mipSubRange.layerCount              = 1;

            // Prepare current mip level as image blit destination
            cmd->cmdImageMemoryBarrier(
                texture.image,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                mipSubRange);

            // Blit from previous level
            cmd->cmdBlitImage(
                texture.image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                texture.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &imageBlit,
                VK_FILTER_LINEAR);

            // Prepare current mip level as image blit source for next level
            cmd->cmdImageMemoryBarrier(
                texture.image,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                mipSubRange);
        }

        cmd->cmdTransitionImageLayout(texture.image,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        _device->endSingleTimeCommands(cmd);
    }

    {
        ImageViewCreateInfo createInfo{};
        createInfo.format                      = FORMAT_R8G8B8A8_SRGB;
        createInfo.viewType                    = IMAGE_VIEW_TYPE_2D;
        createInfo.subresourceRange.levelCount = texMipLevels;
        _device->createImageView(&createInfo, &texture.imageView, texture.image);
    }

    {
        // TODO
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.maxLod              = texMipLevels;
        // samplerInfo.maxAnisotropy       = _device->getPhysicalDevice()->getDeviceEnabledFeatures().samplerAnisotropy ? _device->getDeviceProperties().limits.maxSamplerAnisotropy : 1.0f;
        // samplerInfo.anisotropyEnable    = _device->getPhysicalDevice()->getDeviceEnabledFeatures().samplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK_RESULT(vkCreateSampler(_device->getHandle(), &samplerInfo, nullptr, &texture.sampler));
    }

    texture.descriptorInfo = vkl::init::descriptorImageInfo(texture.sampler, texture.imageView->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    _device->destroyBuffer(stagingBuffer);

    return texture;
}

} // namespace vkl
