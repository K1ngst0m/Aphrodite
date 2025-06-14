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

    auto* pResourceLoader = new ResourceLoader(createInfo);
    if (!pResourceLoader)
    {
        return { Result::RuntimeError, "Failed to allocate ResourceLoader instance" };
    }

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
    pResourceLoader->cleanup();
    delete pResourceLoader;
}

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo)
    : m_createInfo(createInfo)
    , m_pDevice(createInfo.pDevice)
    , m_shaderLoader(m_pDevice)
    , m_geometryLoader(this)
    , m_imageLoader(this)
    , m_bufferLoader(this)
    , m_materialLoader(createInfo.pMaterialRegistry)
{
}

auto ResourceLoader::initialize(const ResourceLoaderCreateInfo& createInfo) -> Result
{
    APH_PROFILER_SCOPE();

    m_pQueue         = m_pDevice->getQueue(QueueType::Transfer);
    m_pGraphicsQueue = m_pDevice->getQueue(QueueType::Graphics);

    if (!m_pQueue || !m_pGraphicsQueue)
    {
        return { Result::RuntimeError, "Failed to get required queues for ResourceLoader" };
    }

    if (!createInfo.pMaterialRegistry)
    {
        APH_LOG_WARN("ResourceLoader initialized without a valid MaterialRegistry");
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

void ResourceLoader::update(const BufferUpdateInfo& info, BufferAsset* pBufferAsset) const
{
    APH_PROFILER_SCOPE();

    if (!pBufferAsset || !pBufferAsset->isValid())
    {
        LOADER_LOG_ERR("Invalid buffer asset provided for update");
        return;
    }

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
    m_geometryLoader.unload(pGeometryAsset);
}

void ResourceLoader::unLoadImpl(ImageAsset* pImageAsset)
{
    APH_PROFILER_SCOPE();
    m_imageLoader.unload(pImageAsset);
}

void ResourceLoader::unLoadImpl(BufferAsset* pBufferAsset)
{
    APH_PROFILER_SCOPE();
    m_bufferLoader.unload(pBufferAsset);
}

void ResourceLoader::unLoadImpl(MaterialAsset* pMaterialAsset)
{
    APH_PROFILER_SCOPE();
    m_materialLoader.unload(pMaterialAsset);
}

auto ResourceLoader::createRequest() -> LoadRequest
{
    LoadRequest request{ this, m_taskManager.createTaskGroup("Load Request"), m_createInfo.async };
    return request;
}

auto ResourceLoader::loadImpl(const GeometryLoadInfo& info) -> Expected<GeometryAsset*>
{
    APH_PROFILER_SCOPE();

    GeometryLoadInfo modifiedInfo = info;

    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to geometry: %s", info.debugName.c_str());
    }

    GeometryAsset* pAsset = {};
    APH_RETURN_IF_ERROR(m_geometryLoader.load(modifiedInfo, &pAsset));
    return { pAsset };
}

auto ResourceLoader::loadImpl(const ImageLoadInfo& info) -> Expected<ImageAsset*>
{
    APH_PROFILER_SCOPE();

    ImageLoadInfo modifiedInfo = info;

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

    BufferLoadInfo modifiedInfo = info;

    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to buffer: %s", info.debugName.c_str());
    }

    BufferAsset* pBufferAsset = nullptr;
    auto result               = m_bufferLoader.load(modifiedInfo, &pBufferAsset);

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

    ShaderLoadInfo modifiedInfo = info;

    if (m_createInfo.forceUncached)
    {
        modifiedInfo.forceUncached = true;
        LOADER_LOG_DEBUG("Propagating global forceUncached=true to shader: %s", info.debugName.c_str());
    }

    ShaderAsset* pShaderAsset = {};
    APH_RETURN_IF_ERROR(m_shaderLoader.load(modifiedInfo, &pShaderAsset));
    return { pShaderAsset };
}

auto ResourceLoader::loadImpl(const MaterialLoadInfo& info) -> Expected<MaterialAsset*>
{
    APH_PROFILER_SCOPE();

    MaterialAsset* pMaterialAsset = nullptr;
    auto result                   = m_materialLoader.load(info, &pMaterialAsset);

    if (!result.success())
    {
        APH_LOG_ERR("Failed to load material: %s", result.toString());
        return { result.getCode(), "Failed to load material asset" };
    }

    return pMaterialAsset;
}

auto ResourceLoader::getDevice() const -> vk::Device*
{
    return m_pDevice;
}

auto LoadRequest::loadAsync() -> std::future<Result>
{
    APH_PROFILER_SCOPE();
    if (!m_async)
    {
        LOADER_LOG_WARN("Async path requested but not available. Falling back to synchronous loading.");
        load();
        std::promise<Result> promise;
        promise.set_value(Result{ Result::Success });
        return promise.get_future();
    }
    return m_pTaskGroup->submitAsync();
}

void LoadRequest::load()
{
    APH_PROFILER_SCOPE();
    APH_VERIFY_RESULT(m_pTaskGroup->submit());
}

LoadRequest::LoadRequest(ResourceLoader* pLoader, TaskGroup* pGroup, bool async)
    : m_pLoader(pLoader)
    , m_pTaskGroup(pGroup)
    , m_async(async)
{
}
} // namespace aph
