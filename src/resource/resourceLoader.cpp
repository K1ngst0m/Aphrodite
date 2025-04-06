#include "resourceLoader.h"

#include "common/common.h"
#include "common/profiler.h"

#include "api/vulkan/device.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

namespace aph
{
ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_pDevice(createInfo.pDevice)
{
    m_pQueue = m_pDevice->getQueue(QueueType::Transfer);
    m_pGraphicsQueue = m_pDevice->getQueue(QueueType::Graphics);
}

ResourceLoader::~ResourceLoader() = default;

void ResourceLoader::cleanup()
{
    APH_PROFILER_SCOPE();
    APH_VERIFY_RESULT(m_pDevice->waitIdle());
    for (auto [res, unLoadCB] : m_unloadQueue)
    {
        unLoadCB();
    }
    m_unloadQueue.clear();
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
            m_pDevice->executeCommand(m_pQueue, [=](auto* cmd) { cmd->update(pBuffer, {0, uploadSize}, info.data); });
        }
        else
        {
            APH_PROFILER_SCOPE_NAME("loading data by: staging copy.");
            for (std::size_t offset = info.range.offset; offset < uploadSize; offset += LIMIT_BUFFER_UPLOAD_SIZE)
            {
                Range copyRange = {
                    .offset = offset,
                    .size = std::min(std::size_t{LIMIT_BUFFER_UPLOAD_SIZE}, {uploadSize - offset}),
                };

                // using staging buffer
                vk::Buffer* stagingBuffer{};
                {
                    vk::BufferCreateInfo stagingCI{
                        .size = copyRange.size,
                        .usage = BufferUsage::TransferSrc,
                        .domain = MemoryDomain::Upload,
                    };

                    auto stagingResult = m_pDevice->create(stagingCI, "staging buffer");
                    APH_VERIFY_RESULT(stagingResult);
                    stagingBuffer = stagingResult.value();

                    writeBuffer(stagingBuffer, info.data, {0, copyRange.size});
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

void ResourceLoader::unLoadImpl(vk::Buffer* pBuffer)
{
    APH_PROFILER_SCOPE();
    m_pDevice->destroy(pBuffer);
}

void ResourceLoader::unLoadImpl(vk::ShaderProgram* pProgram)
{
    APH_PROFILER_SCOPE();
    m_pDevice->destroy(pProgram);
}

void ResourceLoader::unLoadImpl(GeometryAsset* pGeometryAsset)
{
    APH_PROFILER_SCOPE();
    m_geometryLoader.destroy(pGeometryAsset);
}

void ResourceLoader::unLoadImpl(ImageAsset* pImageAsset)
{
    APH_PROFILER_SCOPE();
    m_imageLoader.destroy(pImageAsset);
}

LoadRequest ResourceLoader::getLoadRequest()
{
    LoadRequest request{this, m_taskManager.createTaskGroup("Load Request"), m_createInfo.async};
    return request;
}

Expected<GeometryAsset*> ResourceLoader::loadImpl(const GeometryLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    GeometryAsset* pAsset = {};
    APH_RETURN_IF_ERROR(m_geometryLoader.loadFromFile(info, &pAsset));
    return {pAsset};
}

Expected<ImageAsset*> ResourceLoader::loadImpl(const ImageLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    ImageAsset* pAsset;
    APH_RETURN_IF_ERROR(m_imageLoader.loadFromFile(info, &pAsset));
    return {pAsset};
}

Expected<vk::Buffer*> ResourceLoader::loadImpl(const BufferLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    vk::BufferCreateInfo bufferCI = info.createInfo;

    vk::Buffer* pBuffer = {};

    {
        bufferCI.usage |= BufferUsage::TransferDst;
        auto bufferResult = m_pDevice->create(bufferCI, info.debugName);
        APH_VERIFY_RESULT(bufferResult);
        pBuffer = bufferResult.value();
    }

    // update buffer
    if (info.data)
    {
        this->update(
            {
                .data = info.data,
                .range = {0, info.createInfo.size},
            },
            &pBuffer);
    }

    return pBuffer;
}

Expected<vk::ShaderProgram*> ResourceLoader::loadImpl(const ShaderLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    vk::ShaderProgram* pProgram = {};
    APH_RETURN_IF_ERROR(m_shaderLoader.load(info, &pProgram));
    return {pProgram};
}
} // namespace aph
