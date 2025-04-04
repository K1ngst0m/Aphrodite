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

struct LoadRequest;
class ResourceLoader
{
public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);
    ~ResourceLoader();

    LoadRequest getLoadRequest();

    template <typename T_LoadInfo, typename T_Resource>
    Result load(T_LoadInfo&& loadInfo, T_Resource** ppResource)
    {
        CM_LOG_DEBUG("Loading begin: [%s]", loadInfo.debugName);
        auto result = loadImpl(std::forward<T_LoadInfo>(loadInfo), ppResource);
        std::lock_guard<std::mutex> lock{m_unloadQueueLock};
        m_unloadQueue[*ppResource] = [this, pResource = *ppResource]() { unLoadImpl(pResource); };
        CM_LOG_DEBUG("Loading end: [%s]", loadInfo.debugName);
        return result;
    }

    template <typename T_Resource>
    void unLoad(T_Resource* pResource)
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

    void update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer);

    void cleanup();

    vk::Device* getDevice() const
    {
        return m_pDevice;
    }

private:
    Result loadImpl(const ImageLoadInfo& info, vk::Image** ppImage);
    Result loadImpl(const BufferLoadInfo& info, vk::Buffer** ppBuffer);
    Result loadImpl(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram);
    Result loadImpl(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset);

    void unLoadImpl(vk::Image* pImage);
    void unLoadImpl(vk::Buffer* pBuffer);
    void unLoadImpl(vk::ShaderProgram* pProgram);
    void unLoadImpl(GeometryAsset* pGeometryAsset);

    void writeBuffer(vk::Buffer* pBuffer, const void* data, Range range = {});

private:
    ResourceLoaderCreateInfo m_createInfo;

    vk::Device* m_pDevice = {};
    vk::Queue* m_pQueue = {};

    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;

private:
    std::mutex m_updateLock;
    std::mutex m_unloadQueueLock;
    HashMap<void*, std::function<void()>> m_unloadQueue;

    ShaderLoader m_shaderLoader{m_pDevice};
    GeometryLoader m_geometryLoader{this};

private:
    static constexpr uint32_t LIMIT_BUFFER_CMD_UPDATE_SIZE = 65536U;
    static constexpr uint32_t LIMIT_BUFFER_UPLOAD_SIZE = 8ull << 20;
};

struct LoadRequest
{
    template <typename T_LoadInfo, typename T_Resource>
    LoadRequest& add(T_LoadInfo loadInfo, T_Resource** ppResource)
    {
        auto loadFunction = [](ResourceLoader* pLoader, T_LoadInfo info, T_Resource** ppRes) -> TaskType
        { co_return pLoader->load(std::move(info), ppRes); };

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
