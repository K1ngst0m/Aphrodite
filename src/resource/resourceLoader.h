#pragma once

#include "api/vulkan/device.h"
#include "bufferLoader.h"
#include "common/hash.h"
#include "geometryAsset.h"
#include "geometryLoader.h"
#include "global/globalManager.h"
#include "imageLoader.h"
#include "shaderLoader.h"
#include "threads/taskManager.h"

namespace aph
{

struct ResourceLoaderCreateInfo
{
    bool async = true;
    vk::Device* pDevice = {};
};

// Type traits to map CreateInfo types to Resource types
template <typename TCreateInfo>
struct ResourceTraits;

template <>
struct ResourceTraits<BufferLoadInfo>
{
    using ResourceType = vk::Buffer;
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
    using ResourceType = vk::ShaderProgram;
};

struct LoadRequest;
class ResourceLoader
{
public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);
    ~ResourceLoader();

    LoadRequest getLoadRequest();

    template <typename T_LoadInfo,
              typename T_Resource = typename ResourceTraits<std::decay_t<T_LoadInfo>>::ResourceType>
    Expected<T_Resource*> load(T_LoadInfo&& loadInfo);

    template <typename T_Resource>
    void unLoad(T_Resource* pResource);

    void update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer);

    void cleanup();

    vk::Device* getDevice() const
    {
        return m_pDevice;
    }

private:
    Expected<GeometryAsset*> loadImpl(const GeometryLoadInfo& info);
    Expected<ImageAsset*> loadImpl(const ImageLoadInfo& info);
    Expected<vk::Buffer*> loadImpl(const BufferLoadInfo& info);
    Expected<vk::ShaderProgram*> loadImpl(const ShaderLoadInfo& info);

    void unLoadImpl(vk::Buffer* pBuffer);
    void unLoadImpl(vk::ShaderProgram* pProgram);
    void unLoadImpl(GeometryAsset* pGeometryAsset);
    void unLoadImpl(ImageAsset* pImageAsset);

    void writeBuffer(vk::Buffer* pBuffer, const void* data, Range range = {});

private:
    ResourceLoaderCreateInfo m_createInfo;

    vk::Device* m_pDevice = {};
    vk::Queue* m_pQueue = {};
    vk::Queue* m_pGraphicsQueue = {};

    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;

private:
    std::mutex m_updateLock;
    std::mutex m_unloadQueueLock;
    HashMap<void*, std::function<void()>> m_unloadQueue;

    ShaderLoader m_shaderLoader{m_pDevice};
    GeometryLoader m_geometryLoader{this};
    ImageLoader m_imageLoader{this};

private:
    static constexpr uint32_t LIMIT_BUFFER_CMD_UPDATE_SIZE = 65536U;
    static constexpr uint32_t LIMIT_BUFFER_UPLOAD_SIZE = 8ull << 20;
};

template <typename T_Resource>
inline void ResourceLoader::unLoad(T_Resource* pResource)
{
    if constexpr (ResourceHandleType<T_Resource>)
    {
        CM_LOG_DEBUG("unLoading begin: [%s]", pResource->getDebugName());
    }
    else
    {
        CM_LOG_DEBUG("unLoading begin");
    }

    APH_ASSERT(pResource);
    APH_ASSERT(m_unloadQueue.contains(pResource));
    if (pResource && m_unloadQueue.contains(pResource))
    {
        unLoadImpl(pResource);
        std::lock_guard<std::mutex> lock{m_unloadQueueLock};
        m_unloadQueue.erase(pResource);
    }

    if constexpr (ResourceHandleType<T_Resource>)
    {
        CM_LOG_DEBUG("unLoading end: [%s]", pResource->getDebugName());
    }
    else
    {
        CM_LOG_DEBUG("unLoading end");
    }
}

template <typename T_LoadInfo, typename T_Resource>
inline Expected<T_Resource*> ResourceLoader::load(T_LoadInfo&& loadInfo)
{
    CM_LOG_DEBUG("Loading begin: [%s]", loadInfo.debugName);
    auto expected = loadImpl(std::forward<T_LoadInfo>(loadInfo));
    if (!expected)
    {
        return expected;
    }
    std::lock_guard<std::mutex> lock{m_unloadQueueLock};
    m_unloadQueue[expected.value()] = [this, pResource = expected.value()]() { unLoadImpl(pResource); };
    CM_LOG_DEBUG("Loading end: [%s]", loadInfo.debugName);
    return expected;
}

struct LoadRequest
{
    template <typename T_LoadInfo, typename T_Resource>
    LoadRequest& add(T_LoadInfo loadInfo, T_Resource** ppResource)
    {
        auto loadFunction = [](ResourceLoader* pLoader, T_LoadInfo info, T_Resource** ppRes) -> TaskType
        {
            auto expected = pLoader->load(std::move(info));
            if (!expected)
            {
                co_return expected;
            }
            *ppRes = expected.value();
            co_return Result::Success;
        };

        m_pTaskGroup->addTask(loadFunction(m_pLoader, loadInfo, ppResource));
        return *this;
    }

    void load()
    {
        APH_PROFILER_SCOPE();
        APH_VERIFY_RESULT(m_pTaskGroup->submit());
    }

    std::future<Result> loadAsync()
    {
        APH_PROFILER_SCOPE();
        if (!m_async)
        {
            CM_LOG_WARN("Async path requested but not available. Falling back to synchronous loading.");
            load();
            std::promise<Result> promise;
            promise.set_value(Result{Result::Success});
            return promise.get_future();
        }
        return m_pTaskGroup->submitAsync();
    }

private:
    friend class ResourceLoader;
    LoadRequest(ResourceLoader* pLoader, TaskGroup* pGroup, bool async)
        : m_pLoader(pLoader)
        , m_pTaskGroup(pGroup)
        , m_async(async)
    {
    }
    ResourceLoader* m_pLoader = {};
    TaskGroup* m_pTaskGroup = {};
    bool m_async = true;
};
} // namespace aph
