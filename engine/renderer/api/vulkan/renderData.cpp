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
        createInfo.format    = FORMAT_R8G8B8A8_UNORM;
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
        createInfo.format                      = FORMAT_R8G8B8A8_UNORM;
        createInfo.viewType                    = IMAGE_VIEW_TYPE_2D;
        createInfo.subresourceRange.levelCount = texMipLevels;
        pDevice->createImageView(&createInfo, &texture.imageView, texture.image);
    }

    {
        VkSamplerCreateInfo samplerInfo = vkl::init::samplerCreateInfo();
        samplerInfo.maxLod              = texMipLevels;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK_RESULT(vkCreateSampler(pDevice->getHandle(), &samplerInfo, nullptr, &texture.sampler));
    }

    texture.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texture.descriptorInfo.imageView   = texture.imageView->getHandle();
    texture.descriptorInfo.sampler     = texture.sampler;

    pDevice->destroyBuffer(stagingBuffer);

    return texture;
}
}

VulkanRenderData::VulkanRenderData(VulkanDevice *device, std::shared_ptr<SceneNode> sceneNode)
    : m_pDevice(device), m_node(std::move(sceneNode)) {

    for (auto &image : m_node->getObject<Entity>()->m_images) {
        // raw image data
        unsigned char *imageData     = image->data.data();
        uint32_t       imageDataSize = image->data.size();
        uint32_t       width         = image->width;
        uint32_t       height        = image->height;

        auto texture = createTexture(m_pDevice, width, height, imageData, imageDataSize, true);
        m_textures.push_back(texture);
    }

    // load buffer
    auto &vertices = m_node->getObject<Entity>()->m_vertices;
    auto &indices  = m_node->getObject<Entity>()->m_indices;

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

VulkanRenderData::~VulkanRenderData()
{
    m_pDevice->destroyBuffer(m_vertexBuffer);
    m_pDevice->destroyBuffer(m_indexBuffer);
    m_pDevice->destroyBuffer(m_objectUB);

    for(auto &texture : m_textures)
    {
        m_pDevice->destroyImage(texture.image);
        m_pDevice->destroyImageView(texture.imageView);
        vkDestroySampler(m_pDevice->getHandle(), texture.sampler, nullptr);
    }
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
