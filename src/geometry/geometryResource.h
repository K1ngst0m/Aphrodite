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

    // Buffer access
    [[nodiscard]] virtual auto getPositionBuffer() const -> vk::Buffer*      = 0;
    [[nodiscard]] virtual auto getAttributeBuffer() const -> vk::Buffer*     = 0;
    [[nodiscard]] virtual auto getIndexBuffer() const -> vk::Buffer*         = 0;
    [[nodiscard]] virtual auto getMeshletBuffer() const -> vk::Buffer*       = 0;
    [[nodiscard]] virtual auto getMeshletVertexBuffer() const -> vk::Buffer* = 0;
    [[nodiscard]] virtual auto getMeshletIndexBuffer() const -> vk::Buffer*  = 0;

    // Statistics access
    [[nodiscard]] virtual auto getVertexCount() const -> uint32_t             = 0;
    [[nodiscard]] virtual auto getIndexCount() const -> uint32_t              = 0;
    [[nodiscard]] virtual auto getMeshletCount() const -> uint32_t            = 0;
    [[nodiscard]] virtual auto getMeshletMaxVertexCount() const -> uint32_t   = 0;
    [[nodiscard]] virtual auto getMeshletMaxTriangleCount() const -> uint32_t = 0;

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

    // Buffer access implementations
    auto getPositionBuffer() const -> vk::Buffer* override;
    auto getAttributeBuffer() const -> vk::Buffer* override;
    auto getIndexBuffer() const -> vk::Buffer* override;
    auto getMeshletBuffer() const -> vk::Buffer* override; // Not used in traditional rendering
    auto getMeshletVertexBuffer() const -> vk::Buffer* override; // Not used in traditional rendering
    auto getMeshletIndexBuffer() const -> vk::Buffer* override; // Not used in traditional rendering

    // Statistics access implementations
    auto getVertexCount() const -> uint32_t override;
    auto getIndexCount() const -> uint32_t override;
    auto getMeshletCount() const -> uint32_t override; // Not used in traditional rendering
    auto getMeshletMaxVertexCount() const -> uint32_t override; // Not used in traditional rendering
    auto getMeshletMaxTriangleCount() const -> uint32_t override; // Not used in traditional rendering
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

    // Buffer access implementations
    auto getPositionBuffer() const -> vk::Buffer* override;
    auto getAttributeBuffer() const -> vk::Buffer* override;
    auto getIndexBuffer() const -> vk::Buffer* override;
    auto getMeshletBuffer() const -> vk::Buffer* override;
    auto getMeshletVertexBuffer() const -> vk::Buffer* override;
    auto getMeshletIndexBuffer() const -> vk::Buffer* override;

    // Statistics access implementations
    auto getVertexCount() const -> uint32_t override;
    auto getIndexCount() const -> uint32_t override;
    auto getMeshletCount() const -> uint32_t override;
    auto getMeshletMaxVertexCount() const -> uint32_t override;
    auto getMeshletMaxTriangleCount() const -> uint32_t override;
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
