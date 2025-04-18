#pragma once

#include "coro/coro.hpp"
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

enum class GeometryAttributeBits : uint8_t
{
    eNone               = 0,
    eGenerateNormals    = 1 << 0,
    eGenerateTangents   = 1 << 1,
    eQuantizeAttributes = 1 << 2
};
using GeometryAttributeFlags = Flags<GeometryAttributeBits>;

template <>
struct FlagTraits<GeometryAttributeBits>
{
    static constexpr bool isBitmask                  = true;
    static constexpr GeometryAttributeFlags allFlags = GeometryAttributeBits::eGenerateNormals |
                                                       GeometryAttributeBits::eGenerateTangents |
                                                       GeometryAttributeBits::eQuantizeAttributes;
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
    GeometryAttributeFlags attributeFlags       = GeometryAttributeBits::eNone;

    // Vertex input layout (needed for traditional rendering)
    VertexInput vertexInput;

    // Meshlet parameters
    uint32_t maxVertsPerMeshlet = 64;
    uint32_t maxPrimsPerMeshlet = 124;

    // Prefer mesh shading if supported by the device
    bool preferMeshShading = true;

    // For future dynamic geometry support
    GeometryUsage usage = GeometryUsage::eStatic;

    // Skip cache check when true
    bool forceUncached = false;
};

// Mid-level geometry asset class that manages both traditional and mesh shader geometry
class GeometryAsset
{
public:
    GeometryAsset();
    GeometryAsset(const GeometryAsset&)                    = delete;
    GeometryAsset(GeometryAsset&&)                         = delete;
    auto operator=(const GeometryAsset&) -> GeometryAsset& = delete;
    auto operator=(GeometryAsset&&) -> GeometryAsset&      = delete;
    ~GeometryAsset();

    // Accessors
    [[nodiscard]] auto submeshes() const -> coro::generator<const Submesh*>;
    [[nodiscard]] auto getSubmeshCount() const -> uint32_t;
    [[nodiscard]] auto getSubmesh(uint32_t index) const -> const Submesh*;
    [[nodiscard]] auto getBoundingBox() const -> BoundingBox;
    [[nodiscard]] auto supportsMeshShading() const -> bool;
    [[nodiscard]] auto getMaterialIndex(uint32_t submeshIndex) const -> uint32_t;
    [[nodiscard]] auto getGeometryResource() const -> IGeometryResource*;

    // Buffer accessors
    [[nodiscard]] auto getPositionBuffer() const -> vk::Buffer*;
    [[nodiscard]] auto getAttributeBuffer() const -> vk::Buffer*;
    [[nodiscard]] auto getIndexBuffer() const -> vk::Buffer*;
    [[nodiscard]] auto getMeshletBuffer() const -> vk::Buffer*;
    [[nodiscard]] auto getMeshletVertexBuffer() const -> vk::Buffer*;
    [[nodiscard]] auto getMeshletIndexBuffer() const -> vk::Buffer*;

    // Statistics accessors
    [[nodiscard]] auto getVertexCount() const -> uint32_t;
    [[nodiscard]] auto getIndexCount() const -> uint32_t;
    [[nodiscard]] auto getMeshletCount() const -> uint32_t;
    [[nodiscard]] auto getMeshletMaxVertexCount() const -> uint32_t;
    [[nodiscard]] auto getMeshletMaxTriangleCount() const -> uint32_t;

    void setMaterialIndex(uint32_t submeshIndex, uint32_t materialIndex);
    void setGeometryResource(std::unique_ptr<IGeometryResource> pResource);

private:
    std::unique_ptr<IGeometryResource> m_pGeometryResource;
};
} // namespace aph
