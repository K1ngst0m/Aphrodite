#pragma once

#include "geometry/geometry.h"
#include "geometry/geometryResource.h"
#include "geometryAsset.h"
#include <functional>

namespace aph
{
// Forward declarations
class ResourceLoader;

// Geometry loader class (internal to the resource system)
class GeometryLoader
{
public:
    GeometryLoader(ResourceLoader* pResourceLoader);
    ~GeometryLoader();

    // Load a geometry asset from a file
    Result loadFromFile(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset);

    // Destroy a geometry asset
    void destroy(GeometryAsset* pGeometryAsset);

private:
    // Helper for loading GLTF models
    Result loadGLTF(const GeometryLoadInfo& info, GeometryAsset** ppGeometryAsset);

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
    Result processGeometry(const std::vector<GLTFMesh>& meshes, const GeometryLoadInfo& info,
                           GeometryAsset** ppGeometryAsset);

    // Create GPU resources for the geometry
    Result createGeometryResources(const std::vector<Meshlet>& meshlets, const std::vector<uint32_t>& meshletVertices,
                                   const std::vector<uint32_t>& meshletIndices, const std::vector<Submesh>& submeshes,
                                   const std::vector<GLTFMesh>& meshes, const GeometryLoadInfo& info,
                                   GeometryAsset** ppGeometryAsset);

private:
    ResourceLoader* m_pResourceLoader;
};

} // namespace aph
