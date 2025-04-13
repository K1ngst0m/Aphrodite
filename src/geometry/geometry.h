#pragma once

#include "api/gpuResource.h"
#include "api/vulkan/buffer.h"
#include "math/boundingVolume.h"

namespace aph
{
// Basic structure for a meshlet - the fundamental unit for both traditional and mesh shader pipelines
struct Meshlet
{
    uint32_t vertexCount; // Number of vertices used by this meshlet
    uint32_t triangleCount; // Number of triangles in this meshlet
    uint32_t vertexOffset; // Offset into meshlet vertex array
    uint32_t triangleOffset; // Offset into meshlet triangle array
    std::array<float, 4> positionBounds; // Bounding sphere: xyz = center, w = radius
    std::array<float, 4> coneCenterAndAngle; // xyz = cone center, w = cone cutoff angle
    uint32_t materialIndex; // Material index for this meshlet
};

// Submesh represents a group of meshlets with the same material
struct Submesh
{
    uint32_t meshletOffset{}; // First meshlet index in the mesh
    uint32_t meshletCount{}; // Number of meshlets in this submesh
    uint32_t materialIndex{}; // Material index used by this submesh
    BoundingBox bounds; // AABB of this submesh
};

// Shared GPU data used by both geometry implementations
struct GeometryGpuData
{
    vk::Buffer* pPositionBuffer        = nullptr;
    vk::Buffer* pAttributeBuffer       = nullptr;
    vk::Buffer* pIndexBuffer           = nullptr;
    vk::Buffer* pMeshletBuffer         = nullptr;
    vk::Buffer* pMeshletVertexBuffer   = nullptr;
    vk::Buffer* pMeshletTriangleBuffer = nullptr;
    vk::Buffer* pDrawCommandBuffer     = nullptr;

    uint32_t vertexCount             = 0;
    uint32_t indexCount              = 0;
    uint32_t meshletCount            = 0;
    uint32_t meshletMaxVertexCount   = 64;
    uint32_t meshletMaxTriangleCount = 124;

    IndexType indexType = IndexType::UINT32;
};

} // namespace aph
