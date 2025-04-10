#pragma once

#include "geometry/geometry.h"
#include "geometry/geometryResource.h"

namespace aph
{
// Enums and flags for geometry
enum class GeometryUsage : uint8_t
{
    Static  = 0,
    Dynamic = 1,
    Skinned = 2,
    Morph   = 3
};

enum class GeometryFeatureBits : uint32_t
{
    None              = 0,
    Shadows           = 1 << 0,
    Collision         = 1 << 1,
    StreamingPriority = 1 << 2,
    StructuredBuffers = 1 << 3,
};
using GeometryFeatureFlags = Flags<GeometryFeatureBits>;

template <>
struct FlagTraits<GeometryFeatureBits>
{
    static constexpr bool isBitmask                = true;
    static constexpr GeometryFeatureFlags allFlags = GeometryFeatureBits::Shadows | GeometryFeatureBits::Collision |
                                                     GeometryFeatureBits::StreamingPriority |
                                                     GeometryFeatureBits::StructuredBuffers;
};

enum class MeshletFeatureBits : uint32_t
{
    None                  = 0,
    CullingData           = 1 << 0,
    OptimizeForGPUCulling = 1 << 1,
    PrimitiveOrdering     = 1 << 2,
    LocalClusterFitting   = 1 << 3,
};
using MeshletFeatureFlags = Flags<MeshletFeatureBits>;

template <>
struct FlagTraits<MeshletFeatureBits>
{
    static constexpr bool isBitmask = true;
    static constexpr MeshletFeatureFlags allFlags =
        MeshletFeatureBits::CullingData | MeshletFeatureBits::OptimizeForGPUCulling |
        MeshletFeatureBits::PrimitiveOrdering | MeshletFeatureBits::LocalClusterFitting;
};

enum class GeometryOptimizationBits : uint32_t
{
    None        = 0,
    VertexCache = 1 << 0,
    Overdraw    = 1 << 1,
    VertexFetch = 1 << 2,
    All         = VertexCache | Overdraw | VertexFetch
};
using GeometryOptimizationFlags = Flags<GeometryOptimizationBits>;

template <>
struct FlagTraits<GeometryOptimizationBits>
{
    static constexpr bool isBitmask                     = true;
    static constexpr GeometryOptimizationFlags allFlags = GeometryOptimizationBits::VertexCache |
                                                          GeometryOptimizationBits::Overdraw |
                                                          GeometryOptimizationBits::VertexFetch;
};

// Mid-level geometry asset class that manages both traditional and mesh shader geometry
class GeometryAsset
{
public:
    GeometryAsset();
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
    GeometryFeatureFlags featureFlags           = GeometryFeatureBits::None;
    MeshletFeatureFlags meshletFlags            = MeshletFeatureBits::CullingData;
    GeometryOptimizationFlags optimizationFlags = GeometryOptimizationBits::All;

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
    GeometryUsage usage = GeometryUsage::Static;
};

} // namespace aph
