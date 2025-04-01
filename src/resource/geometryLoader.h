#pragma once

#include "api/vulkan/device.h"

namespace aph
{
class ResourceLoader;

struct Geometry
{
    static constexpr uint32_t MAX_VERTEX_BINDINGS = 15;

    std::vector<vk::Buffer*> indexBuffer;
    IndexType indexType;

    std::vector<vk::Buffer*> vertexBuffers;
    std::vector<uint32_t> vertexStrides;

    std::vector<DrawIndexArguments*> drawArgs;
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
} // namespace aph

namespace aph::loader::geometry
{
bool loadGLTF(aph::ResourceLoader* pLoader, const aph::GeometryLoadInfo& info, aph::Geometry** ppGeometry);
} // namespace aph::loader::geometry
