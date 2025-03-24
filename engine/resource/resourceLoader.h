#pragma once

#include "api/vulkan/device.h"
#include "bufferLoader.h"
#include "common/hash.h"
#include "geometryLoader.h"
#include "imageLoader.h"
#include "shaderLoader.h"
#include "threads/taskManager.h"
#include <format>

namespace aph
{

struct ResourceLoaderCreateInfo
{
    // TODO for debugging
    bool isMultiThreads = false;
    vk::Device* pDevice = {};
};

class ResourceLoader
{
    enum
    {
        LIMIT_BUFFER_CMD_UPDATE_SIZE = 65536,
        LIMIT_BUFFER_UPLOAD_SIZE = 8ull << 20,
    };

public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);

    ~ResourceLoader();

    template <typename T_LoadInfo, typename T_Resource>
    std::future<Result> loadAsync(const T_LoadInfo& loadInfo, T_Resource** ppResource)
    {
        auto taskGroup = m_taskManager.createTaskGroup(
            std::format("Loading [{}]", loadInfo.debugName.empty() ? "unnamed" : loadInfo.debugName));

        auto task = taskGroup->addTask(
            std::function<Result()>{ [this, loadInfo, ppResource]() { return load(loadInfo, ppResource); } });

        taskGroup->submit();
        return task->getResult();
    }

    template <typename T_LoadInfo, typename T_Resource>
    Result load(const T_LoadInfo& loadInfo, T_Resource** ppResource)
    {
        auto result = loadImpl(loadInfo, ppResource);
        m_unloadQueue[*ppResource] = [this, pResource = *ppResource]() { unLoadImpl(pResource); };
        return result;
    }

    template <typename T_Resource>
    void unLoad(T_Resource* pResource)
    {
        APH_ASSERT(pResource);
        APH_ASSERT(m_unloadQueue.contains(pResource));
        if (pResource && m_unloadQueue.contains(pResource))
        {
            unLoadImpl(pResource);
            m_unloadQueue.erase(pResource);
        }
    }

    void wait()
    {
        m_taskManager.wait();
    }

    void update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer);

    void cleanup();

private:
    Result loadImpl(const ImageLoadInfo& info, vk::Image** ppImage);
    Result loadImpl(const BufferLoadInfo& info, vk::Buffer** ppBuffer);
    Result loadImpl(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram);
    Result loadImpl(const GeometryLoadInfo& info, Geometry** ppGeometry);

    void unLoadImpl(vk::Image* pImage);
    void unLoadImpl(vk::Buffer* pBuffer);
    void unLoadImpl(vk::ShaderProgram* pProgram);
    void unLoadImpl(Geometry* pGeometry);

    void writeBuffer(vk::Buffer* pBuffer, const void* data, Range range = {});

private:
    ResourceLoaderCreateInfo m_createInfo;
    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;
    vk::Device* m_pDevice = {};
    vk::Queue* m_pQueue = {};

private:
    std::mutex m_updateLock;
    HashMap<void*, std::function<void()>> m_unloadQueue;

    ShaderLoader m_shaderLoader{ m_pDevice };
};
} // namespace aph
