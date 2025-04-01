#include "resourceLoader.h"

#include "common/common.h"
#include "common/profiler.h"

#include "api/vulkan/device.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

namespace aph
{
ImageContainerType GetImageContainerType(const std::filesystem::path& path)
{
    APH_PROFILER_SCOPE();
    if (path.extension() == ".ktx")
    {
        return ImageContainerType::Ktx;
    }

    if (path.extension() == ".png")
    {
        return ImageContainerType::Png;
    }

    if (path.extension() == ".jpg")
    {
        return ImageContainerType::Jpg;
    }

    CM_LOG_ERR("Unsupported image format.");

    return ImageContainerType::Default;
}

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_pDevice(createInfo.pDevice)
{
    m_pQueue = m_pDevice->getQueue(QueueType::Transfer);
}

ResourceLoader::~ResourceLoader() = default;

void ResourceLoader::cleanup()
{
    APH_PROFILER_SCOPE();
    APH_VR(m_pDevice->waitIdle());
    for (auto [res, unLoadCB] : m_unloadQueue)
    {
        unLoadCB();
    }
    m_unloadQueue.clear();
}

Result ResourceLoader::loadImpl(const ImageLoadInfo& info, vk::Image** ppImage)
{
    APH_PROFILER_SCOPE();
    std::filesystem::path path;
    std::vector<uint8_t> data;
    vk::ImageCreateInfo ci;
    ci = info.createInfo;

    if (std::holds_alternative<std::string>(info.data))
    {
        auto& filesystem = APH_DEFAULT_FILESYSTEM;
        path = filesystem.resolvePath(std::get<std::string>(info.data));

        auto containerType =
            info.containerType == ImageContainerType::Default ? GetImageContainerType(path) : info.containerType;

        switch (containerType)
        {
        case ImageContainerType::Ktx:
        {
            loadKTX(path, ci, data);
        }
        break;
        case ImageContainerType::Png:
        case ImageContainerType::Jpg:
        {
            loadPNGJPG(path, ci, data, info.isFlipY);
        }
        break;
        case ImageContainerType::Default:
            APH_ASSERT(false);
            return { Result::RuntimeError, "Unsupported image type." };
        }
    }
    else if (std::holds_alternative<ImageInfo>(info.data))
    {
        auto img = std::get<ImageInfo>(info.data);
        data = img.data;
        ci.extent = { img.width, img.height, 1 };
    }

    // Load texture from image buffer
    vk::Buffer* stagingBuffer;
    {
        vk::BufferCreateInfo bufferCI{
            .size = data.size(),
            .usage = BufferUsage::TransferSrc,
            .domain = MemoryDomain::Upload,
        };
        APH_VR(m_pDevice->create(bufferCI, &stagingBuffer, std::string{ info.debugName } + std::string{ "_staging" }));

        writeBuffer(stagingBuffer, data.data());
    }

    vk::Image* image{};

    {
        bool genMipmap = ci.mipLevels > 1 || info.generateMips;

        auto imageCI = ci;
        imageCI.usage |= ImageUsage::TransferDst;
        imageCI.domain = MemoryDomain::Device;
        if (genMipmap)
        {
            imageCI.usage |= ImageUsage::TransferSrc;
            // If generating mipmaps dynamically, set the mipmap count
            if (info.generateMips && imageCI.mipLevels <= 1)
            {
                uint32_t width = imageCI.extent.width;
                uint32_t height = imageCI.extent.height;
                uint32_t mipLevels = 1;

                // Calculate how many mip levels we can generate
                while (width > 1 || height > 1)
                {
                    width = std::max(1u, width / 2);
                    height = std::max(1u, height / 2);
                    mipLevels++;
                }

                imageCI.mipLevels = mipLevels;
            }
        }

        APH_VR(m_pDevice->create(imageCI, &image, info.debugName));

        auto queue = m_pQueue;

        // mip map opeartions
        m_pDevice->executeCommand(
            queue,
            [&](auto* cmd)
            {
                cmd->transitionImageLayout(image, ResourceState::Undefined, ResourceState::CopyDest);
                cmd->copy(stagingBuffer, image);

                if (genMipmap)
                {
                    cmd->transitionImageLayout(image, ResourceState::CopyDest, ResourceState::CopySource);
                    int32_t width = ci.extent.width;
                    int32_t height = ci.extent.height;

                    // generate mipmap chains
                    for (int32_t i = 1; i < imageCI.mipLevels; i++)
                    {
                        vk::ImageBlitInfo srcBlitInfo{
                            .extent = { int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1 },
                            .level = static_cast<uint32_t>(i - 1),
                            .layerCount = 1,
                        };

                        vk::ImageBlitInfo dstBlitInfo{
                            .extent = { int32_t(width >> i), int32_t(height >> i), 1 },
                            .level = static_cast<uint32_t>(i),
                            .layerCount = 1,
                        };

                        // Prepare current mip level as image blit destination
                        vk::ImageBarrier barrier{
                            .pImage = image,
                            .currentState = ResourceState::CopySource,
                            .newState = ResourceState::CopyDest,
                            .subresourceBarrier = 1,
                            .mipLevel = static_cast<uint8_t>(imageCI.mipLevels),
                        };
                        cmd->insertBarrier({ barrier });

                        // Blit from previous level
                        cmd->blit(image, image, srcBlitInfo, dstBlitInfo);

                        barrier.currentState = ResourceState::CopyDest;
                        barrier.newState = ResourceState::CopySource;
                        cmd->insertBarrier({ barrier });
                    }
                }

                // Final transition to ShaderResource
                cmd->transitionImageLayout(image, genMipmap ? ResourceState::CopySource : ResourceState::CopyDest,
                                           ResourceState::ShaderResource);
            });
    }

    m_pDevice->destroy(stagingBuffer);
    *ppImage = image;

    return Result::Success;
}

Result ResourceLoader::loadImpl(const BufferLoadInfo& info, vk::Buffer** ppBuffer)
{
    APH_PROFILER_SCOPE();
    vk::BufferCreateInfo bufferCI = info.createInfo;

    {
        bufferCI.usage |= BufferUsage::TransferDst;
        APH_VR(m_pDevice->create(bufferCI, ppBuffer, info.debugName));
    }

    // update buffer
    if (info.data)
    {
        this->update(
            {
                .data = info.data,
                .range = { 0, info.createInfo.size },
            },
            ppBuffer);
    }

    return Result::Success;
}

Result ResourceLoader::loadImpl(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram)
{
    APH_PROFILER_SCOPE();
    return m_shaderLoader.load(info, ppProgram);
}

Result ResourceLoader::loadImpl(const GeometryLoadInfo& info, Geometry** ppGeometry)
{
    APH_PROFILER_SCOPE();
    auto path = std::filesystem::path{ info.path };
    auto ext = path.extension();

    if (ext == ".glb" || ext == ".gltf")
    {
        loader::geometry::loadGLTF(this, info, ppGeometry);
    }
    else
    {
        CM_LOG_ERR("Unsupported model file type: %s.", ext);
        APH_ASSERT(false);
        return { Result::RuntimeError, "Unsupported model file type." };
    }
    return Result::Success;
}

void ResourceLoader::update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer)
{
    APH_PROFILER_SCOPE();
    vk::Buffer* pBuffer = *ppBuffer;
    MemoryDomain domain = pBuffer->getCreateInfo().domain;
    std::size_t uploadSize = info.range.size;

    // device only
    if (domain == MemoryDomain::Device)
    {
        if (info.range.size == VK_WHOLE_SIZE)
        {
            uploadSize = pBuffer->getSize();
        }

        if (uploadSize <= LIMIT_BUFFER_CMD_UPDATE_SIZE)
        {
            APH_PROFILER_SCOPE_NAME("loading data by: vkCmdBufferUpdate.");
            m_pDevice->executeCommand(m_pQueue, [=](auto* cmd) { cmd->update(pBuffer, { 0, uploadSize }, info.data); });
        }
        else
        {
            APH_PROFILER_SCOPE_NAME("loading data by: staging copy.");
            for (std::size_t offset = info.range.offset; offset < uploadSize; offset += LIMIT_BUFFER_UPLOAD_SIZE)
            {
                Range copyRange = {
                    .offset = offset,
                    .size = std::min(std::size_t{ LIMIT_BUFFER_UPLOAD_SIZE }, { uploadSize - offset }),
                };

                // using staging buffer
                vk::Buffer* stagingBuffer{};
                {
                    vk::BufferCreateInfo stagingCI{
                        .size = copyRange.size,
                        .usage = BufferUsage::TransferSrc,
                        .domain = MemoryDomain::Upload,
                    };

                    APH_VR(m_pDevice->create(stagingCI, &stagingBuffer, "staging buffer"));

                    writeBuffer(stagingBuffer, info.data, { 0, copyRange.size });
                }

                m_pDevice->executeCommand(m_pQueue, [=](auto* cmd) { cmd->copy(stagingBuffer, pBuffer, copyRange); });

                m_pDevice->destroy(stagingBuffer);
            }
        }
    }
    else
    {
        APH_PROFILER_SCOPE_NAME("loading data by: vkMapMemory.");
        writeBuffer(pBuffer, info.data, info.range);
    }
}

void ResourceLoader::writeBuffer(vk::Buffer* pBuffer, const void* data, Range range)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(pBuffer->getCreateInfo().domain != MemoryDomain::Device);
    if (range.size == 0)
    {
        range.size = VK_WHOLE_SIZE;
    }

    if (range.size == VK_WHOLE_SIZE || range.size == 0)
    {
        range.size = pBuffer->getSize();
    }

    void* pMapped = m_pDevice->mapMemory(pBuffer);
    APH_ASSERT(pMapped);
    std::memcpy((uint8_t*)pMapped + range.offset, data, range.size);
    m_pDevice->unMapMemory(pBuffer);
}
void ResourceLoader::unLoadImpl(vk::Image* pImage)
{
    m_pDevice->destroy(pImage);
}
void ResourceLoader::unLoadImpl(vk::Buffer* pBuffer)
{
    m_pDevice->destroy(pBuffer);
}
void ResourceLoader::unLoadImpl(vk::ShaderProgram* pProgram)
{
    m_pDevice->destroy(pProgram);
}
void ResourceLoader::unLoadImpl(Geometry* pGeometry)
{
    for (auto* pBuffer : pGeometry->indexBuffer)
    {
        m_pDevice->destroy(pBuffer);
    }
    for (auto* pBuffer : pGeometry->vertexBuffers)
    {
        m_pDevice->destroy(pBuffer);
    }
}
LoadRequest ResourceLoader::getLoadRequest()
{
    LoadRequest request{ this, m_taskManager.createTaskGroup("Load Request"), m_createInfo.async };
    return request;
}
} // namespace aph
