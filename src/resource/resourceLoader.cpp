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

void ResourceLoader::update(const BufferUpdateInfo& info, BufferAsset* pBufferAsset)
{
    APH_PROFILER_SCOPE();

    if (!pBufferAsset || !pBufferAsset->isValid())
    {
        CM_LOG_ERR("Invalid buffer asset provided for update");
        return;
    }

    // Use the buffer asset's update method
    auto result = pBufferAsset->update(info);
    if (!result)
    {
        CM_LOG_ERR("Failed to update buffer: %s", result.toString());
    }
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

void ResourceLoader::unLoadImpl(BufferAsset* pBufferAsset)
{
    APH_PROFILER_SCOPE();
    m_bufferLoader.destroy(pBufferAsset);
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

Expected<BufferAsset*> ResourceLoader::loadImpl(const BufferLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    BufferAsset* pBufferAsset = nullptr;
    auto result = m_bufferLoader.loadFromData(info, &pBufferAsset);

    if (!result)
    {
        CM_LOG_ERR("Failed to load buffer: %s", result.toString());
        return result;
    }

    return pBufferAsset;
}

Expected<vk::ShaderProgram*> ResourceLoader::loadImpl(const ShaderLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    vk::ShaderProgram* pProgram = {};
    APH_RETURN_IF_ERROR(m_shaderLoader.load(info, &pProgram));
    return {pProgram};
}
} // namespace aph
