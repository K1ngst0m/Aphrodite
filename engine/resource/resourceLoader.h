#pragma once

#include "api/vulkan/device.h"
#include "threads/taskManager.h"
#include "common/hash.h"
#include "imageLoader.h"
#include "shaderLoader.h"
#include "bufferLoader.h"
#include "geometryLoader.h"
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

    void wait()
    {
        m_taskManager.wait();
    }

    Result load(const ImageLoadInfo& info, vk::Image** ppImage);
    Result load(const BufferLoadInfo& info, vk::Buffer** ppBuffer);
    Result load(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram);
    Result load(const GeometryLoadInfo& info, Geometry** ppGeometry);
    void update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer);

    void cleanup();

private:
    void writeBuffer(vk::Buffer* pBuffer, const void* data, Range range = {});

private:
    ResourceLoaderCreateInfo m_createInfo;
    TaskManager& m_taskManager = APH_DEFAULT_TASK_MANAGER;
    vk::Device* m_pDevice = {};
    vk::Queue* m_pQueue = {};

private:
    HashMap<std::string, HashMap<ShaderStage, vk::Shader*>> m_shaderCaches = {};
    std::mutex m_updateLock;
};
} // namespace aph
