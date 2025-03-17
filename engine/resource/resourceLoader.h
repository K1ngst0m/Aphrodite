#pragma once

#include "api/vulkan/device.h"
#include "common/hash.h"
#include "geometry.h"
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

enum class ImageContainerType
{
    Default = 0,
    Ktx,
    Png,
    Jpg,
};

struct ImageInfo
{
    uint32_t width = {};
    uint32_t height = {};
    std::vector<uint8_t> data = {};
};

struct ImageLoadInfo
{
    std::string debugName = {};
    std::variant<std::string, ImageInfo> data;
    ImageContainerType containerType = { ImageContainerType::Default };
    vk::ImageCreateInfo createInfo = {};
};

struct BufferLoadInfo
{
    std::string debugName = {};
    const void* data = {};
    vk::BufferCreateInfo createInfo = {};
};

struct BufferUpdateInfo
{
    const void* data = {};
    Range range = { 0, VK_WHOLE_SIZE };
};

struct ShaderStageLoadInfo
{
    std::variant<std::string, std::vector<uint32_t>> data;
    std::vector<ShaderMacro> macros;
    std::string entryPoint = "main";
};

struct ShaderLoadInfo
{
    std::string debugName = {};
    HashMap<ShaderStage, ShaderStageLoadInfo> stageInfo;
    std::vector<ShaderConstant> constants;
    vk::BindlessResource* pBindlessResource = {};
};

enum GeometryLoadFlags
{
    GEOMETRY_LOAD_FLAG_SHADOWED = 0x1,
    GEOMETRY_LOAD_FLAG_STRUCTURED_BUFFERS = 0x2,
};

enum MeshOptimizerFlags
{
    MESH_OPTIMIZATION_FLAG_OFF = 0x0,
    MESH_OPTIMIZATION_FLAG_VERTEXCACHE = 0x1,
    MESH_OPTIMIZATION_FLAG_OVERDRAW = 0x2,
    MESH_OPTIMIZATION_FLAG_VERTEXFETCH = 0x4,
    MESH_OPTIMIZATION_FLAG_ALL = 0x7,
};

struct GeometryLoadInfo
{
    std::string path;
    GeometryLoadFlags flags;
    MeshOptimizerFlags optimizationFlags;
    VertexInput vertexInput;
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
