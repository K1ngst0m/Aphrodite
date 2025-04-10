#pragma once

#include "geometry/geometry.h"
#include "geometry/geometryResource.h"

namespace aph
{
// Enums and flags for geometry
enum class GeometryUsage : uint8_t
{
    eStatic  = 0,
    eDynamic = 1,
    eSkinned = 2,
    eMorph   = 3
};

enum class GeometryFeatureBits : uint8_t
{
    eNone              = 0,
    eShadows           = 1 << 0,
    eCollision         = 1 << 1,
    eStreamingPriority = 1 << 2,
    eStructuredBuffers = 1 << 3,
};
using GeometryFeatureFlags = Flags<GeometryFeatureBits>;

template <>
struct FlagTraits<GeometryFeatureBits>
{
    static constexpr bool isBitmask                = true;
    static constexpr GeometryFeatureFlags allFlags = GeometryFeatureBits::eShadows | GeometryFeatureBits::eCollision |
                                                     GeometryFeatureBits::eStreamingPriority |
                                                     GeometryFeatureBits::eStructuredBuffers;
};

enum class MeshletFeatureBits : uint8_t
{
    eNone                  = 0,
    eCullingData           = 1 << 0,
    eOptimizeForGPUCulling = 1 << 1,
    ePrimitiveOrdering     = 1 << 2,
    eLocalClusterFitting   = 1 << 3,
};
using MeshletFeatureFlags = Flags<MeshletFeatureBits>;

template <>
struct FlagTraits<MeshletFeatureBits>
{
    static constexpr bool isBitmask = true;
    static constexpr MeshletFeatureFlags allFlags =
        MeshletFeatureBits::eCullingData | MeshletFeatureBits::eOptimizeForGPUCulling |
        MeshletFeatureBits::ePrimitiveOrdering | MeshletFeatureBits::eLocalClusterFitting;
};

enum class GeometryOptimizationBits : uint8_t
{
    eNone        = 0,
    eVertexCache = 1 << 0,
    eOverdraw    = 1 << 1,
    eVertexFetch = 1 << 2,
    eAll         = eVertexCache | eOverdraw | eVertexFetch
};
using GeometryOptimizationFlags = Flags<GeometryOptimizationBits>;

template <>
struct FlagTraits<GeometryOptimizationBits>
{
    static constexpr bool isBitmask                     = true;
    static constexpr GeometryOptimizationFlags allFlags = GeometryOptimizationBits::eVertexCache |
                                                          GeometryOptimizationBits::eOverdraw |
                                                          GeometryOptimizationBits::eVertexFetch;
};

// Mid-level geometry asset class that manages both traditional and mesh shader geometry
class GeometryAsset
{
public:
    GeometryAsset();
    GeometryAsset(const GeometryAsset&)            = delete;
    GeometryAsset(GeometryAsset&&)                 = delete;
    GeometryAsset& operator=(const GeometryAsset&) = delete;
    GeometryAsset& operator=(GeometryAsset&&)      = delete;
    ~GeometryAsset();

    // Accessors
    uint32_t getSubmeshCount() const;
    const Submesh* getSubmesh(uint32_t index) const;
    BoundingBox getBoundingBox() const;
    bool supportsMeshShading() const;

    // Draw operations
    void bind(vk::CommandBuffer* cmdBuffer);
    void draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex = 0, uint32_t instanceCount = 1);

    // Material assignments - to be used by scene system
    void setMaterialIndex(uint32_t submeshIndex, uint32_t materialIndex);
    uint32_t getMaterialIndex(uint32_t submeshIndex) const;

    // Internal use by the geometry loader
    void setGeometryResource(std::unique_ptr<IGeometryResource> pResource);

private:
    std::unique_ptr<IGeometryResource> m_pGeometryResource;
};

// Load info structure for geometry
struct GeometryLoadInfo
{
    // Path to the model file (currently GLTF)
    std::string path;
    std::string debugName;

    // Flags and options
    GeometryFeatureFlags featureFlags           = GeometryFeatureBits::eNone;
    MeshletFeatureFlags meshletFlags            = MeshletFeatureBits::eCullingData;
    GeometryOptimizationFlags optimizationFlags = GeometryOptimizationBits::eAll;

    // Vertex input layout (needed for traditional rendering)
    VertexInput vertexInput;

    // Meshlet parameters
    uint32_t maxVertsPerMeshlet = 64;
    uint32_t maxPrimsPerMeshlet = 124;

    // Prefer mesh shading if supported by the device
    bool preferMeshShading = true;

    // Mesh processing options
    bool generateNormals    = false;
    bool generateTangents   = false;
    bool quantizeAttributes = false;

    // For future dynamic geometry support
    GeometryUsage usage = GeometryUsage::eStatic;
};

} // namespace aph
