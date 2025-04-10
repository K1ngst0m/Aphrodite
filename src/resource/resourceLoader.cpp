#include "resourceLoader.h"

#include "common/common.h"
#include "common/profiler.h"

#include "api/vulkan/device.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"
#include "shaderAsset.h"

namespace aph
{
Expected<ResourceLoader*> ResourceLoader::Create(const ResourceLoaderCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Create ResourceLoader with minimal initialization in constructor
    auto* pResourceLoader = new ResourceLoader(createInfo);
    if (!pResourceLoader)
    {
        return {Result::RuntimeError, "Failed to allocate ResourceLoader instance"};
    }

    // Complete initialization
    Result initResult = pResourceLoader->initialize(createInfo);
    if (!initResult.success())
    {
        delete pResourceLoader;
        return {initResult.getCode(), initResult.toString()};
    }

    return pResourceLoader;
}

void ResourceLoader::Destroy(ResourceLoader* pResourceLoader)
{
    if (!pResourceLoader)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    // Clean up resources
    pResourceLoader->cleanup();

    // Delete the instance
    delete pResourceLoader;
}

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_pDevice(createInfo.pDevice)
{
}

Result ResourceLoader::initialize(const ResourceLoaderCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Initialize queues
    m_pQueue         = m_pDevice->getQueue(QueueType::Transfer);
    m_pGraphicsQueue = m_pDevice->getQueue(QueueType::Graphics);

    if (!m_pQueue || !m_pGraphicsQueue)
    {
        return {Result::RuntimeError, "Failed to get required queues for ResourceLoader"};
    }

    return Result::Success;
}

void ResourceLoader::cleanup()
{
    APH_PROFILER_SCOPE();
    APH_VERIFY_RESULT(m_pDevice->waitIdle());
    for (const auto& [res, unLoadCB] : m_unloadQueue)
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

void ResourceLoader::unLoadImpl(ShaderAsset* pShaderAsset)
{
    APH_PROFILER_SCOPE();
    if (pShaderAsset && pShaderAsset->isValid())
    {
        m_pDevice->destroy(pShaderAsset->getProgram());
    }
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

LoadRequest ResourceLoader::createRequest()
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
    return m_imageLoader.loadFromFile(info);
}

Expected<BufferAsset*> ResourceLoader::loadImpl(const BufferLoadInfo& info)
{
    APH_PROFILER_SCOPE();

    BufferAsset* pBufferAsset = nullptr;
    auto result               = m_bufferLoader.loadFromData(info, &pBufferAsset);

    if (!result)
    {
        CM_LOG_ERR("Failed to load buffer: %s", result.toString());
        return result;
    }

    return pBufferAsset;
}

Expected<ShaderAsset*> ResourceLoader::loadImpl(const ShaderLoadInfo& info)
{
    APH_PROFILER_SCOPE();
    ShaderAsset* pShaderAsset = {};
    APH_RETURN_IF_ERROR(m_shaderLoader.load(info, &pShaderAsset));
    return {pShaderAsset};
}
} // namespace aph
