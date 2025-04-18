#pragma once

#include "allocator/objectPool.h"
#include "geometry/geometry.h"
#include "geometry/geometryResource.h"
#include "geometryAsset.h"
#include "resource/forward.h"
#include <functional>

namespace aph
{
class ResourceLoader;

class GeometryLoader
{
public:
    explicit GeometryLoader(ResourceLoader* pResourceLoader);
    ~GeometryLoader();

    auto load(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset) -> Result;
    void unload(GeometryAsset* pGeometryAsset);

private:
    auto loadGLTF(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset) -> Result;

    // Helper for converting tinygltf data into our format
    struct GLTFMesh
    {
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> tangents;
        std::vector<float> texcoords0;
        std::vector<float> texcoords1;
        std::vector<float> colors;
        std::vector<uint32_t> indices;
        uint32_t materialIndex;
    };

    // Process vertex data to optimize it
    auto processGeometry(const std::vector<GLTFMesh>& meshes, const GeometryLoadInfo& info,
                         GeometryAsset** ppGeometryAsset) -> Result;

    auto createGeometryResources(const std::vector<Meshlet>& meshlets, const std::vector<uint32_t>& meshletVertices,
                                 const std::vector<uint32_t>& meshletIndices, const std::vector<Submesh>& submeshes,
                                 const std::vector<GLTFMesh>& meshes, const GeometryLoadInfo& info,
                                 GeometryAsset** ppGeometryAsset) -> Result;

private:
    ResourceLoader* m_pResourceLoader;
    ThreadSafeObjectPool<GeometryAsset> m_geometryAssetPool;
};

} // namespace aph
