#include "resourceLoader.h"

#include "common/common.h"
#include "common/profiler.h"

#include "api/vulkan/device.h"
#include "filesystem/filesystem.h"
#include "global/globalManager.h"

namespace aph
{
auto ResourceLoader::Create(const ResourceLoaderCreateInfo& createInfo) -> Expected<ResourceLoader*>
{
    APH_PROFILER_SCOPE();

    // Create ResourceLoader with minimal initialization in constructor
    auto* pResourceLoader = new ResourceLoader(createInfo);
    if (!pResourceLoader)
    {
        return { Result::RuntimeError, "Failed to allocate ResourceLoader instance" };
    }

    // Complete initialization
    Result initResult = pResourceLoader->initialize(createInfo);
    if (!initResult.success())
    {
        delete pResourceLoader;
        return { initResult.getCode(), initResult.toString() };
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

auto ResourceLoader::initialize(const ResourceLoaderCreateInfo& createInfo) -> Result
{
    APH_PROFILER_SCOPE();

    // Initialize queues
    m_pQueue         = m_pDevice->getQueue(QueueType::Transfer);
    m_pGraphicsQueue = m_pDevice->getQueue(QueueType::Graphics);

    if (!m_pQueue || !m_pGraphicsQueue)
    {
        return { Result::RuntimeError, "Failed to get required queues for ResourceLoader" };
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
        LOADER_LOG_ERR("Invalid buffer asset provided for update");
        return;
    }

    // Use the buffer asset's update method
    auto result = pBufferAsset->update(info);
    if (!result)
    {
        LOADER_LOG_ERR("Failed to update buffer: %s", result.toString());
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
    m_imageLoader.unload(pImageAsset);
}

void ResourceLoader::unLoadImpl(BufferAsset* pBufferAsset)
{
    APH_PROFILER_SCOPE();
    m_bufferLoader.destroy(pBufferAsset);
}

auto ResourceLoader::createRequest() -> LoadRequest
{
    LoadRequest request{ this, m_taskManager.createTaskGroup("Load Request"), m_createInfo.async };
    return request;
}

auto ResourceLoader::loadImpl(const GeometryLoadInfo& info) -> Expected<GeometryAsset*>
{
    APH_PROFILER_SCOPE();

    // Copy info to apply resource loader settings
    GeometryLoadInfo modifiedInfo = info;

    // Propagate forceUncached setting from ResourceLoader to GeometryLoader
    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to geometry: %s", info.debugName.c_str());
    }

    GeometryAsset* pAsset = {};
    APH_RETURN_IF_ERROR(m_geometryLoader.loadFromFile(modifiedInfo, &pAsset));
    return { pAsset };
}

auto ResourceLoader::loadImpl(const ImageLoadInfo& info) -> Expected<ImageAsset*>
{
    APH_PROFILER_SCOPE();

    // Copy info to apply resource loader settings
    ImageLoadInfo modifiedInfo = info;

    // Propagate forceUncached setting from ResourceLoader to ImageLoader
    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to image: %s", info.debugName.c_str());
    }

    return m_imageLoader.load(modifiedInfo);
}

auto ResourceLoader::loadImpl(const BufferLoadInfo& info) -> Expected<BufferAsset*>
{
    APH_PROFILER_SCOPE();

    // Copy info to apply resource loader settings
    BufferLoadInfo modifiedInfo = info;

    // Propagate forceUncached setting from ResourceLoader to BufferLoader
    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to buffer: %s", info.debugName.c_str());
    }

    BufferAsset* pBufferAsset = nullptr;
    auto result               = m_bufferLoader.loadFromData(modifiedInfo, &pBufferAsset);

    if (!result)
    {
        LOADER_LOG_ERR("Failed to load buffer: %s", result.toString());
        return result;
    }

    return pBufferAsset;
}

auto ResourceLoader::loadImpl(const ShaderLoadInfo& info) -> Expected<ShaderAsset*>
{
    APH_PROFILER_SCOPE();

    // Copy info to apply resource loader settings
    ShaderLoadInfo modifiedInfo = info;

    // Propagate forceUncached setting from ResourceLoader to ShaderLoader
    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to shader: %s", info.debugName.c_str());
    }

    ShaderAsset* pShaderAsset = {};
    APH_RETURN_IF_ERROR(m_shaderLoader.load(modifiedInfo, &pShaderAsset));
    return { pShaderAsset };
}
} // namespace aph
