#include "renderData.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "descriptorSetLayout.h"
#include "device.h"
#include "image.h"
#include "imageView.h"
#include "pipeline.h"
#include "scene/entity.h"
#include "sceneRenderer.h"
#include "vkInit.hpp"

namespace vkl {
namespace  {
TextureGpuData createTexture(VulkanDevice * pDevice, uint32_t width, uint32_t height, void *data, uint32_t dataSize, bool genMipmap = false) {
    uint32_t texMipLevels = genMipmap ? calculateFullMipLevels(width, height) : 1;

    // Load texture from image buffer
    vkl::VulkanBuffer *stagingBuffer;
    {
        BufferCreateInfo createInfo{};
        createInfo.size     = dataSize;
        createInfo.usage    = BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
        pDevice->createBuffer(&createInfo, &stagingBuffer);

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
        createInfo.usage     = IMAGE_USAGE_TRANSFER_SRC_BIT | IMAGE_USAGE_TRANSFER_DST_BIT | IMAGE_USAGE_SAMPLED_BIT;
        createInfo.property  = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        createInfo.mipLevels = texMipLevels;

        pDevice->createImage(&createInfo, &texture.image);

        auto *cmd = pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cmd->cmdCopyBufferToImage(stagingBuffer, texture.image);
        cmd->cmdTransitionImageLayout(texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        pDevice->endSingleTimeCommands(cmd);

        cmd = pDevice->beginSingleTimeCommands(VK_QUEUE_GRAPHICS_BIT);

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

        pDevice->endSingleTimeCommands(cmd);
    }

    {
        ImageViewCreateInfo createInfo{};
        createInfo.format                      = FORMAT_R8G8B8A8_SRGB;
        createInfo.viewType                    = IMAGE_VIEW_TYPE_2D;
        createInfo.subresourceRange.levelCount = texMipLevels;
        pDevice->createImageView(&createInfo, &texture.imageView, texture.image);
    }

    {
        // TODO
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.maxLod              = texMipLevels;
        // samplerInfo.maxAnisotropy       = _device->getPhysicalDevice()->getDeviceEnabledFeatures().samplerAnisotropy ? _device->getDeviceProperties().limits.maxSamplerAnisotropy : 1.0f;
        // samplerInfo.anisotropyEnable    = _device->getPhysicalDevice()->getDeviceEnabledFeatures().samplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK_RESULT(vkCreateSampler(pDevice->getHandle(), &samplerInfo, nullptr, &texture.sampler));
    }

    pDevice->destroyBuffer(stagingBuffer);

    return texture;
}
}

void TextureGpuData::setupDescriptor() {
    descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorInfo.imageView   = imageView->getHandle();
    descriptorInfo.sampler     = sampler;
}

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
    : m_pDevice(device), m_node(std::move(sceneNode)) {
    // create empty texture
    {
        uint32_t width         = 1024;
        uint32_t height        = 1024;
        uint32_t imageDataSize = width * height * 4;

        std::vector<uint8_t> data(imageDataSize, 0);
        m_emptyTexture = createTexture(m_pDevice, width, height, data.data(), imageDataSize);
        m_emptyTexture.setupDescriptor();
    }

    for (auto &image : m_node->getObject<Entity>()->_images) {
        // raw image data
        unsigned char *imageData     = image->data.data();
        uint32_t       imageDataSize = image->data.size();
        uint32_t       width         = image->width;
        uint32_t       height        = image->height;

        auto texture = createTexture(m_pDevice, width, height, imageData, imageDataSize, true);
        m_textures.push_back(texture);
        m_textures.back().setupDescriptor();
    }

    // load buffer
    auto &vertices = m_node->getObject<Entity>()->_vertices;
    auto &indices  = m_node->getObject<Entity>()->_indices;

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
            m_pDevice->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(vertices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage    = BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            m_pDevice->createBuffer(&createInfo, &m_vertexBuffer);
        }

        auto cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
        m_pDevice->endSingleTimeCommands(cmd);

        m_pDevice->destroyBuffer(stagingBuffer);
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
            m_pDevice->createBuffer(&createInfo, &stagingBuffer);
        }

        stagingBuffer->map();
        stagingBuffer->copyTo(indices.data(), static_cast<VkDeviceSize>(bufferSize));
        stagingBuffer->unmap();

        {
            BufferCreateInfo createInfo{};
            createInfo.size     = bufferSize;
            createInfo.property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            createInfo.usage    = BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            m_pDevice->createBuffer(&createInfo, &m_indexBuffer);
        }

        auto cmd = m_pDevice->beginSingleTimeCommands(VK_QUEUE_TRANSFER_BIT);
        cmd->cmdCopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
        m_pDevice->endSingleTimeCommands(cmd);

        m_pDevice->destroyBuffer(stagingBuffer);
    }

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
        VK_CHECK_RESULT(m_pDevice->createBuffer(&bufferCI, &m_objectUB, &objInfo));
        m_objectUB->setupDescriptor();
    }

    {
        m_objectSet = objectLayout->allocateSet();

        std::vector<VkWriteDescriptorSet> descriptorWrites{
            vkl::init::writeDescriptorSet(m_objectSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &m_objectUB->getBufferInfo())
        };

        vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    for (auto &material : m_node->getObject<Entity>()->_materials) {
        // write descriptor set
        auto set = materialLayout->allocateSet();
        VulkanBuffer * matInfoUB = nullptr;
        {
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
                    .metallicRoughnessTextureIndex = material.metallicRoughnessTextureIndex,
                    .specularGlossinessTextureIndex = material.specularGlossinessTextureIndex,
                };
                BufferCreateInfo bufferCI{
                    .size = sizeof(MaterialInfo),
                    .alignment = 0,
                    .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT,
                };
                m_pDevice->createBuffer(&bufferCI, &matInfoUB, &matInfo);
                matInfoUB->setupDescriptor();
            }
            std::vector<VkWriteDescriptorSet> descriptorWrites{};
            // create material buffer Info
            descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &matInfoUB->getBufferInfo()));

            if (bindingBits & MATERIAL_BINDING_BASECOLOR) {
                std::cerr << "material id: [" << material.id << "] [base color]: ";
                if (material.baseColorTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &m_textures[material.baseColorTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &m_emptyTexture.descriptorInfo));
                    std::cerr << "texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_NORMAL) {
                std::cerr << "material id: [" << material.id << "] [normal]: ";
                if (material.normalTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &m_textures[material.normalTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &m_emptyTexture.descriptorInfo));
                    std::cerr << "texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_PHYSICAL){
                std::cerr << "material id: [" << material.id << "] [physical desc]: ";
                if (material.metallicFactor > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &m_textures[material.metallicRoughnessTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &m_emptyTexture.descriptorInfo));
                    std::cerr << "texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_AO){
                std::cerr << "material id: [" << material.id << "] [ao]: ";
                if (material.occlusionTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &m_textures[material.occlusionTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &m_emptyTexture.descriptorInfo));
                    std::cerr << "texture not found, use default texture." << std::endl;
                }
            }
            if (bindingBits & MATERIAL_BINDING_EMISSIVE){
                std::cerr << "material id: [" << material.id << "] [emissive]: ";
                if (material.emissiveTextureIndex > -1) {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &m_textures[material.emissiveTextureIndex].descriptorInfo));
                    std::cerr << descriptorWrites.back().pImageInfo->imageView << std::endl;
                } else {
                    descriptorWrites.push_back(vkl::init::writeDescriptorSet(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, &m_emptyTexture.descriptorInfo));
                    std::cerr << "texture not found, use default texture." << std::endl;
                }
            }

            vkUpdateDescriptorSets(m_pDevice->getHandle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        m_materialGpuDataList.push_back({matInfoUB, set});
    }
}

void VulkanRenderData::draw(VulkanPipeline * pipeline, VulkanCommandBuffer *drawCmd) {
    VkDeviceSize offsets[1] = {0};
    drawCmd->cmdBindVertexBuffers(0, 1, m_vertexBuffer, offsets);
    drawCmd->cmdBindIndexBuffers(m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    std::queue<std::shared_ptr<MeshNode>> q;
    q.push(m_node->getObject<Entity>()->m_rootNode);

    while(!q.empty()){
        auto subNode = q.front();
        q.pop();

        glm::mat4 nodeMatrix    = subNode->matrix;
        auto     currentParent = subNode->parent;
        while (currentParent) {
            nodeMatrix    = currentParent->matrix * nodeMatrix;
            currentParent = currentParent->parent;
        }
        nodeMatrix = m_node->matrix * nodeMatrix;
        drawCmd->cmdPushConstants(pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);

        for (const auto& subset : subNode->subsets) {
            if (subset.indexCount > 0) {
                auto &materialSet = m_materialGpuDataList[subset.materialIndex];
                drawCmd->cmdBindDescriptorSet(pipeline, 2, 1, &materialSet.set);
                drawCmd->cmdDrawIndexed(subset.indexCount, 1, subset.firstIndex, 0, 0);
            }
        }

        for (const auto &child : subNode->children){
            q.push(child);
        }
    }
}

uint32_t VulkanRenderData::getSetCount() {
    return m_node->getObject<Entity>()->_materials.size();
}

VulkanRenderData::~VulkanRenderData()
{
    m_pDevice->destroyBuffer(m_vertexBuffer);
    m_pDevice->destroyBuffer(m_indexBuffer);
    m_pDevice->destroyBuffer(m_objectUB);

    for(auto &matData : m_materialGpuDataList)
    {
        m_pDevice->destroyBuffer(matData.buffer);
    }

    for(auto &texture : m_textures)
    {
        m_pDevice->destroyImage(texture.image);
        m_pDevice->destroyImageView(texture.imageView);
        vkDestroySampler(m_pDevice->getHandle(), texture.sampler, nullptr);
    }

    m_pDevice->destroyImage(m_emptyTexture.image);
    m_pDevice->destroyImageView(m_emptyTexture.imageView);
    vkDestroySampler(m_pDevice->getHandle(), m_emptyTexture.sampler, nullptr);
}

VulkanUniformData::VulkanUniformData(VulkanDevice * device, std::shared_ptr<SceneNode> node)
    : m_device(device), m_node(std::move(node)) {
    switch (m_node->attachType) {
    case AttachType::LIGHT:
        m_object = m_node->getObject<Light>();
    case AttachType::CAMERA:
        m_object = m_node->getObject<Camera>();
    default:
        assert("invalid object type");
    }
    m_object->load();

    BufferCreateInfo createInfo{};
    createInfo.size = m_object->getDataSize();
    createInfo.usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.property = MEMORY_PROPERTY_HOST_VISIBLE_BIT | MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VK_CHECK_RESULT(m_device->createBuffer(&createInfo, &m_buffer, m_object->getData()));
    m_buffer->setupDescriptor();
    m_buffer->map();
}

VulkanUniformData::~VulkanUniformData()
{
    m_device->destroyBuffer(m_buffer);
};


}  // namespace vkl
