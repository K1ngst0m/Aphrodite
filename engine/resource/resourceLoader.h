#ifndef RES_LOADER_H_
#define RES_LOADER_H_

#include "common/common.h"
#include "api/vulkan/device.h"
#include "geometry.h"

namespace aph
{

struct ResourceLoaderCreateInfo
{
    // TODO
    bool        isMultiThreads = false;
    vk::Device* pDevice        = {};
};

enum class ImageContainerType
{
    Default = 0,
    Dds,
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
    vk::ImageCreateInfo*                 pCreateInfo   = {};
};

struct BufferLoadInfo
{
    std::string_view     debugName  = {};
    void*                data       = {};
    vk::BufferCreateInfo createInfo = {};
};

struct BufferUpdateInfo
{
    void*            data      = {};
    MemoryRange      range     = {0, VK_WHOLE_SIZE};
    std::string_view debugName = {};
};

struct ShaderLoadInfo
{
    std::variant<std::string, std::vector<uint32_t>> data;
    std::string                                      entryPoint = "main";
    std::vector<ShaderMacro>                         macros;
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
        LIMIT_BUFFER_UPLOAD_SIZE = 8ull << 20,
    };

public:
    ResourceLoader(const ResourceLoaderCreateInfo& createInfo);

    ~ResourceLoader();

    template <typename T_CreateInfo, typename T_Resource>
    void loadAsync(const T_CreateInfo& info, T_Resource** ppResource)
    {
        m_syncTokens.push_back(m_threadPool.enqueue([this, &info, ppResource]() { load(info, ppResource); }));
        // TODO command pool multi thread support
        wait();
    }

    void wait()
    {
        for(auto& syncToken : m_syncTokens)
        {
            syncToken.wait();
        }
    }

    void load(const ImageLoadInfo& info, vk::Image** ppImage);
    void load(const BufferLoadInfo& info, vk::Buffer** ppBuffer);
    void load(const ShaderLoadInfo& info, vk::Shader** ppShader);
    void load(const GeometryLoadInfo& info, Geometry** ppGeometry);
    void update(const BufferUpdateInfo& info, vk::Buffer** ppBuffer);

    void cleanup();

private:
    void writeBuffer(vk::Buffer* pBuffer, const void* data, MemoryRange range = {});

private:
    ResourceLoaderCreateInfo m_createInfo;
    vk::Device*              m_pDevice = {};

private:
    ThreadPool<>                                                 m_threadPool;
    std::unordered_map<std::string, std::unique_ptr<vk::Shader>> m_shaderModuleCaches = {};
    uuid::UUIDGenerator<std::mt19937_64>                         m_uuidGenerator      = {};
    std::unordered_map<std::string, std::string>                 m_shaderUUIDMap      = {};
    std::vector<std::future<void>>                               m_syncTokens;
};
}  // namespace aph

#endif
