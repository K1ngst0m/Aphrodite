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

    // Commands to bind and draw the geometry
    virtual void bind(vk::CommandBuffer* cmdBuffer)                                                        = 0;
    virtual void draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex = 0, uint32_t instanceCount = 1) = 0;

    // Information access
    virtual uint32_t getSubmeshCount() const                = 0;
    virtual const Submesh* getSubmesh(uint32_t index) const = 0;
    virtual BoundingBox getBoundingBox() const              = 0;

    // Query to determine what pipeline to use
    virtual bool supportsMeshShading() const = 0;
};

// Traditional vertex/index-based geometry implementation
class VertexGeometryResource : public IGeometryResource
{
public:
    VertexGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData, const std::vector<Submesh>& submeshes,
                           const VertexInput& vertexInput,
                           PrimitiveTopology topology = PrimitiveTopology::TriangleList);

    ~VertexGeometryResource() override = default;

    void bind(vk::CommandBuffer* cmdBuffer) override;
    void draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex = 0, uint32_t instanceCount = 1) override;

    uint32_t getSubmeshCount() const override
    {
        return static_cast<uint32_t>(m_submeshes.size());
    }
    const Submesh* getSubmesh(uint32_t index) const override
    {
        return &m_submeshes[index];
    }
    BoundingBox getBoundingBox() const override
    {
        return m_boundingBox;
    }

    bool supportsMeshShading() const override
    {
        return false;
    }

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

    void bind(vk::CommandBuffer* cmdBuffer) override;
    void draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex = 0, uint32_t instanceCount = 1) override;

    uint32_t getSubmeshCount() const override
    {
        return static_cast<uint32_t>(m_submeshes.size());
    }
    const Submesh* getSubmesh(uint32_t index) const override
    {
        return &m_submeshes[index];
    }
    BoundingBox getBoundingBox() const override
    {
        return m_boundingBox;
    }

    bool supportsMeshShading() const override
    {
        return true;
    }

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
    static std::unique_ptr<IGeometryResource> createGeometryResource(vk::Device* pDevice,
                                                                     const GeometryGpuData& gpuData,
                                                                     const std::vector<Submesh>& submeshes,
                                                                     const VertexInput& vertexInput,
                                                                     bool preferMeshShading = true);
};

} // namespace aph
