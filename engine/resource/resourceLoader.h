#ifndef RES_LOADER_H_
#define RES_LOADER_H_

#include "api/vulkan/device.h"
#include "common/hash.h"
#include "common/uuid.h"
#include "geometry.h"
#include "threads/taskManager.h"

namespace aph
{

struct ResourceLoaderCreateInfo
{
    // TODO for debugging
    bool        isMultiThreads = false;
    vk::Device* pDevice        = {};
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
    uint32_t             width  = {};
    uint32_t             height = {};
    std::vector<uint8_t> data   = {};
};

struct ImageLoadInfo
{
    std::string_view                     debugName = {};
    std::variant<std::string, ImageInfo> data;
    ImageContainerType                   containerType = {ImageContainerType::Default};
    vk::ImageCreateInfo                  createInfo    = {};
};

struct BufferLoadInfo
{
    std::string_view     debugName  = {};
    const void*          data       = {};
    vk::BufferCreateInfo createInfo = {};
};

struct BufferUpdateInfo
{
    const void*      data      = {};
    MemoryRange      range     = {0, VK_WHOLE_SIZE};
    std::string_view debugName = {};
};

struct ShaderStageLoadInfo
{
    std::variant<std::string, std::vector<uint32_t>> data;
    std::vector<ShaderMacro>                         macros;
    std::string                                      entryPoint = "main";
};

struct ShaderLoadInfo
{
    HashMap<ShaderStage, ShaderStageLoadInfo> stageInfo;
    std::vector<ShaderConstant>               constants;
};

enum GeometryLoadFlags
{
    GEOMETRY_LOAD_FLAG_SHADOWED           = 0x1,
    GEOMETRY_LOAD_FLAG_STRUCTURED_BUFFERS = 0x2,
};

enum MeshOptimizerFlags
{
    MESH_OPTIMIZATION_FLAG_OFF         = 0x0,
    MESH_OPTIMIZATION_FLAG_VERTEXCACHE = 0x1,
    MESH_OPTIMIZATION_FLAG_OVERDRAW    = 0x2,
    MESH_OPTIMIZATION_FLAG_VERTEXFETCH = 0x4,
    MESH_OPTIMIZATION_FLAG_ALL         = 0x7,
};

struct GeometryLoadInfo
{
    std::string        path;
    GeometryLoadFlags  flags;
    MeshOptimizerFlags optimizationFlags;
    VertexInput        vertexInput;
};

class ResourceLoader
{
    enum
    {
        LIMIT_BUFFER_CMD_UPDATE_SIZE = 65536,
        LIMIT_BUFFER_UPLOAD_SIZE     = 8ull << 20,
    };

public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);

    ~ResourceLoader();

    template <typename T_CreateInfo, typename T_Resource>
    Result loadAsync(const T_CreateInfo& info, T_Resource** ppResource)
    {
        Result result = Result::Success;
        auto taskGroup = m_taskManager.createTaskGroup("resource loader.");
        taskGroup->addTask([this, info, ppResource, &result]() { result = load(info, ppResource); });
        taskGroup->submit();
        return result;
    }

    void wait() { m_taskManager.wait(); }

    Result load(const ImageLoadInfo& info, vk::Image** ppImage);
    Result load(const BufferLoadInfo& info, vk::Buffer** ppBuffer);
    Result load(const ShaderLoadInfo& info, vk::ShaderProgram** ppProgram);
    Result load(const GeometryLoadInfo& info, Geometry** ppGeometry);
    void update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer);

    void cleanup();

private:
    void        writeBuffer(vk::Buffer* pBuffer, const void* data, MemoryRange range = {});
    vk::Shader* loadShader(const std::vector<uint32_t>& spv);

private:
    ResourceLoaderCreateInfo m_createInfo;
    TaskManager              m_taskManager = {5, "Resource Loader"};
    vk::Device*              m_pDevice     = {};
    vk::Queue*               m_pQueue      = {};

private:
    ThreadSafeObjectPool<vk::Shader> m_shaderPool;
    HashMap<std::string, HashMap<ShaderStage, vk::Shader*>> m_shaderCaches       = {};
    uuid::UUIDGenerator<std::mt19937_64> m_uuidGenerator      = {};
    HashMap<std::string, std::string>    m_shaderUUIDMap      = {};
    std::mutex                           m_updateLock;
};
}  // namespace aph

#endif
