#pragma once

#include "api/vulkan/device.h"
#include "buffer/bufferLoader.h"
#include "common/hash.h"
#include "common/result.h"
#include "exception/errorMacros.h"
#include "forward.h"
#include "geometry/geometryAsset.h"
#include "geometry/geometryLoader.h"
#include "global/globalManager.h"
#include "image/imageLoader.h"
#include "shader/shaderAsset.h"
#include "shader/shaderLoader.h"
#include "threads/taskManager.h"

namespace aph
{

struct ResourceLoaderCreateInfo
{
    bool async          = true;
    bool forceUncached  = false;
    vk::Device* pDevice = {};
};

// Type traits to map CreateInfo types to Resource types
template <typename TCreateInfo>
struct ResourceTraits;

template <>
struct ResourceTraits<BufferLoadInfo>
{
    using ResourceType = BufferAsset;
};

template <>
struct ResourceTraits<ImageLoadInfo>
{
    using ResourceType = ImageAsset;
};

template <>
struct ResourceTraits<GeometryLoadInfo>
{
    using ResourceType = GeometryAsset;
};

template <>
struct ResourceTraits<ShaderLoadInfo>
{
    using ResourceType = ShaderAsset;
};

struct LoadRequest
{
    template <typename TLoadInfo, typename TResource>
    auto add(TLoadInfo loadInfo, TResource** ppResource) -> LoadRequest&;
    void load();
    auto loadAsync() -> std::future<Result>;

private:
    friend class ResourceLoader;
    LoadRequest(ResourceLoader* pLoader, TaskGroup* pGroup, bool async);
    ResourceLoader* m_pLoader = {};
    TaskGroup* m_pTaskGroup   = {};
    bool m_async              = true;
};

class ResourceLoader
{
private:
    explicit ResourceLoader(const ResourceLoaderCreateInfo& createInfo);
    ~ResourceLoader() = default;
    auto initialize(const ResourceLoaderCreateInfo& createInfo) -> Result;

public:
    ResourceLoader(const ResourceLoader&)                    = delete;
    ResourceLoader(ResourceLoader&&)                         = delete;
    auto operator=(const ResourceLoader&) -> ResourceLoader& = delete;
    auto operator=(ResourceLoader&&) -> ResourceLoader&      = delete;

    // Factory methods
    static auto Create(const ResourceLoaderCreateInfo& createInfo) -> Expected<ResourceLoader*>;
    static void Destroy(ResourceLoader* pResourceLoader);

    auto createRequest() -> LoadRequest;

    template <typename TLoadInfo, typename TResource = typename ResourceTraits<std::decay_t<TLoadInfo>>::ResourceType>
    auto load(TLoadInfo&& loadInfo) -> Expected<TResource*>;

    template <typename TResource>
    void unLoad(TResource* pResource);

    void update(const BufferUpdateInfo& info, BufferAsset* pBufferAsset);

    void cleanup();

    auto getDevice() const -> vk::Device*;

private:
    auto loadImpl(const GeometryLoadInfo& info) -> Expected<GeometryAsset*>;
    auto loadImpl(const ImageLoadInfo& info) -> Expected<ImageAsset*>;
    auto loadImpl(const BufferLoadInfo& info) -> Expected<BufferAsset*>;
    auto loadImpl(const ShaderLoadInfo& info) -> Expected<ShaderAsset*>;

    void unLoadImpl(BufferAsset* pBufferAsset);
    void unLoadImpl(ShaderAsset* pShaderAsset);
    void unLoadImpl(GeometryAsset* pGeometryAsset);
    void unLoadImpl(ImageAsset* pImageAsset);

private:
    ResourceLoaderCreateInfo m_createInfo;

    vk::Device* m_pDevice       = {};
    vk::Queue* m_pQueue         = {};
    vk::Queue* m_pGraphicsQueue = {};

    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;

private:
    std::mutex m_updateLock;
    std::mutex m_unloadQueueLock;
    HashMap<void*, std::function<void()>> m_unloadQueue;

    ShaderLoader m_shaderLoader{ m_pDevice };
    GeometryLoader m_geometryLoader{ this };
    ImageLoader m_imageLoader{ this };
    BufferLoader m_bufferLoader{ this };
};

template <typename TResource>
inline void ResourceLoader::unLoad(TResource* pResource)
{
    if constexpr (ResourceHandleType<TResource>)
    {
        LOADER_LOG_DEBUG("unLoading begin: [%s]", pResource->getDebugName());
    }
    else
    {
        LOADER_LOG_DEBUG("unLoading begin");
    }

    APH_ASSERT(pResource);
    APH_ASSERT(m_unloadQueue.contains(pResource));
    if (pResource && m_unloadQueue.contains(pResource))
    {
        unLoadImpl(pResource);
        std::lock_guard<std::mutex> lock{ m_unloadQueueLock };
        m_unloadQueue.erase(pResource);
    }

    if constexpr (ResourceHandleType<TResource>)
    {
        LOADER_LOG_DEBUG("unLoading end: [%s]", pResource->getDebugName());
    }
    else
    {
        LOADER_LOG_DEBUG("unLoading end");
    }
}

template <typename TLoadInfo, typename TResource>
inline auto ResourceLoader::load(TLoadInfo&& loadInfo) -> Expected<TResource*>
{
    LOADER_LOG_DEBUG("Loading begin: [%s]", loadInfo.debugName);
    auto expected = loadImpl(std::forward<TLoadInfo>(loadInfo));
    if (!expected)
    {
        return expected;
    }
    std::lock_guard<std::mutex> lock{ m_unloadQueueLock };
    m_unloadQueue[expected.value()] = [this, pResource = expected.value()]()
    {
        unLoadImpl(pResource);
    };
    LOADER_LOG_DEBUG("Loading end: [%s]", loadInfo.debugName);
    return expected;
}

template <typename TLoadInfo, typename TResource>
inline auto LoadRequest::add(TLoadInfo loadInfo, TResource** ppResource) -> LoadRequest&
{
    auto loadFunction = [](ResourceLoader* pLoader, TLoadInfo info, TResource** ppRes) -> TaskType
    {
        auto expected = pLoader->load(std::move(info));
        VerifyExpected(expected);
        *ppRes = expected.value();
        co_return Result::Success;
    };

    m_pTaskGroup->addTask(loadFunction(m_pLoader, loadInfo, ppResource));
    return *this;
}

} // namespace aph
