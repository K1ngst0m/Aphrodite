#pragma once

#include "api/vulkan/buffer.h"
#include "api/vulkan/device.h"
#include "geometry.h"

namespace aph
{
// Interface for low-level geometry resources (to be implemented by Vulkan-specific code)
class IGeometryResource
{
public:
    virtual ~IGeometryResource() = default;

    // Information access
    [[nodiscard]] virtual auto getSubmeshCount() const -> uint32_t                = 0;
    [[nodiscard]] virtual auto getSubmesh(uint32_t index) const -> const Submesh* = 0;
    [[nodiscard]] virtual auto getBoundingBox() const -> BoundingBox              = 0;

    // Query to determine what pipeline to use
    [[nodiscard]] virtual auto supportsMeshShading() const -> bool = 0;
};

// Traditional vertex/index-based geometry implementation
class VertexGeometryResource : public IGeometryResource
{
public:
    VertexGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData, const std::vector<Submesh>& submeshes,
                           const VertexInput& vertexInput,
                           PrimitiveTopology topology = PrimitiveTopology::TriangleList);

    ~VertexGeometryResource() override = default;

    auto getSubmeshCount() const -> uint32_t override;
    auto getSubmesh(uint32_t index) const -> const Submesh* override;
    auto getBoundingBox() const -> BoundingBox override;
    auto supportsMeshShading() const -> bool override;

private:
    [[maybe_unused]] vk::Device* m_pDevice;
    GeometryGpuData m_gpuData;
    std::vector<Submesh> m_submeshes;
    VertexInput m_vertexInput;
    PrimitiveTopology m_topology;
    BoundingBox m_boundingBox;
};

// Mesh shader-based geometry implementation
class MeshletGeometryResource : public IGeometryResource
{
public:
    MeshletGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData, const std::vector<Submesh>& submeshes,
                            uint32_t meshletMaxVertexCount = 64, uint32_t meshletMaxTriangleCount = 124);

    ~MeshletGeometryResource() override = default;

    auto getSubmeshCount() const -> uint32_t override;
    auto getSubmesh(uint32_t index) const -> const Submesh* override;
    auto getBoundingBox() const -> BoundingBox override;
    auto supportsMeshShading() const -> bool override;

private:
    vk::Device* m_pDevice;
    GeometryGpuData m_gpuData;
    std::vector<Submesh> m_submeshes;
    BoundingBox m_boundingBox;
    uint32_t m_meshletMaxVertexCount;
    uint32_t m_meshletMaxTriangleCount;
};

// Factory that creates the appropriate geometry resource based on device capabilities
class GeometryResourceFactory
{
public:
    static auto createGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData,
                                       const std::vector<Submesh>& submeshes, const VertexInput& vertexInput,
                                       bool preferMeshShading = true) -> std::unique_ptr<IGeometryResource>;
};

} // namespace aph
