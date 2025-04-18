#include "geometryResource.h"

#include "common/profiler.h"

#include "api/vulkan/vkUtils.h"

namespace aph
{

//
// VertexGeometryResource implementation
//
VertexGeometryResource::VertexGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData,
                                               const std::vector<Submesh>& submeshes, const VertexInput& vertexInput,
                                               PrimitiveTopology topology)
    : m_pDevice(pDevice)
    , m_gpuData(gpuData)
    , m_submeshes(submeshes)
    , m_vertexInput(vertexInput)
    , m_topology(topology)
{
    // Calculate the overall bounding box
    if (!submeshes.empty())
    {
        m_boundingBox = submeshes[0].bounds;
        for (size_t i = 1; i < submeshes.size(); ++i)
        {
            m_boundingBox.extend(submeshes[i].bounds);
        }
    }
}

//
// MeshletGeometryResource implementation
//
MeshletGeometryResource::MeshletGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData,
                                                 const std::vector<Submesh>& submeshes, uint32_t meshletMaxVertexCount,
                                                 uint32_t meshletMaxTriangleCount)
    : m_pDevice(pDevice)
    , m_gpuData(gpuData)
    , m_submeshes(submeshes)
    , m_meshletMaxVertexCount(meshletMaxVertexCount)
    , m_meshletMaxTriangleCount(meshletMaxTriangleCount)
{
    // Calculate the overall bounding box
    if (!submeshes.empty())
    {
        m_boundingBox = submeshes[0].bounds;
        for (size_t i = 1; i < submeshes.size(); ++i)
        {
            m_boundingBox.extend(submeshes[i].bounds);
        }
    }
}

auto MeshletGeometryResource::getSubmeshCount() const -> uint32_t
{
    return static_cast<uint32_t>(m_submeshes.size());
}

auto MeshletGeometryResource::getSubmesh(uint32_t index) const -> const Submesh*
{
    return &m_submeshes[index];
}

auto MeshletGeometryResource::getBoundingBox() const -> BoundingBox
{
    return m_boundingBox;
}

auto MeshletGeometryResource::supportsMeshShading() const -> bool
{
    return true;
}

auto VertexGeometryResource::supportsMeshShading() const -> bool
{
    return false;
}

auto VertexGeometryResource::getBoundingBox() const -> BoundingBox
{
    return m_boundingBox;
}

auto VertexGeometryResource::getSubmesh(uint32_t index) const -> const Submesh*
{
    return &m_submeshes[index];
}

auto VertexGeometryResource::getSubmeshCount() const -> uint32_t
{
    return static_cast<uint32_t>(m_submeshes.size());
}

//
// GeometryResourceFactory implementation
//
auto GeometryResourceFactory::createGeometryResource(vk::Device* pDevice, const GeometryGpuData& gpuData,
                                                     const std::vector<Submesh>& submeshes,
                                                     const VertexInput& vertexInput, bool preferMeshShading)
    -> std::unique_ptr<IGeometryResource>
{
    // Check if mesh shading is supported and preferred
    bool useMeshShading = preferMeshShading && pDevice->getPhysicalDevice()->getProperties().feature.meshShading;

    if (useMeshShading)
    {
        // Create mesh shader based geometry resource
        return std::make_unique<MeshletGeometryResource>(pDevice, gpuData, submeshes, gpuData.meshletMaxVertexCount,
                                                         gpuData.meshletMaxTriangleCount);
    }

    // Create traditional vertex/index based geometry resource
    return std::make_unique<VertexGeometryResource>(pDevice, gpuData, submeshes, vertexInput);
}

auto VertexGeometryResource::getPositionBuffer() const -> vk::Buffer*
{
    return m_gpuData.pPositionBuffer;
}

auto VertexGeometryResource::getAttributeBuffer() const -> vk::Buffer*
{
    return m_gpuData.pAttributeBuffer;
}

auto VertexGeometryResource::getIndexBuffer() const -> vk::Buffer*
{
    return m_gpuData.pIndexBuffer;
}

auto VertexGeometryResource::getMeshletBuffer() const -> vk::Buffer*
{
    return nullptr;
}

auto VertexGeometryResource::getMeshletVertexBuffer() const -> vk::Buffer*
{
    return nullptr;
}

auto VertexGeometryResource::getMeshletIndexBuffer() const -> vk::Buffer*
{
    return nullptr;
}

auto VertexGeometryResource::getVertexCount() const -> uint32_t
{
    return m_gpuData.vertexCount;
}

auto VertexGeometryResource::getIndexCount() const -> uint32_t
{
    return m_gpuData.indexCount;
}

auto VertexGeometryResource::getMeshletCount() const -> uint32_t
{
    return 0;
}

auto VertexGeometryResource::getMeshletMaxVertexCount() const -> uint32_t
{
    return 0;
}

auto VertexGeometryResource::getMeshletMaxTriangleCount() const -> uint32_t
{
    return 0;
}

auto MeshletGeometryResource::getPositionBuffer() const -> vk::Buffer*
{
    return m_gpuData.pPositionBuffer;
}

auto MeshletGeometryResource::getAttributeBuffer() const -> vk::Buffer*
{
    return m_gpuData.pAttributeBuffer;
}

auto MeshletGeometryResource::getIndexBuffer() const -> vk::Buffer*
{
    return m_gpuData.pIndexBuffer;
}

auto MeshletGeometryResource::getMeshletBuffer() const -> vk::Buffer*
{
    return m_gpuData.pMeshletBuffer;
}

auto MeshletGeometryResource::getMeshletVertexBuffer() const -> vk::Buffer*
{
    return m_gpuData.pMeshletVertexBuffer;
}

auto MeshletGeometryResource::getMeshletIndexBuffer() const -> vk::Buffer*
{
    return m_gpuData.pMeshletIndexBuffer;
}

auto MeshletGeometryResource::getVertexCount() const -> uint32_t
{
    return m_gpuData.vertexCount;
}

auto MeshletGeometryResource::getIndexCount() const -> uint32_t
{
    return m_gpuData.indexCount;
}

auto MeshletGeometryResource::getMeshletCount() const -> uint32_t
{
    return m_gpuData.meshletCount;
}

auto MeshletGeometryResource::getMeshletMaxVertexCount() const -> uint32_t
{
    return m_meshletMaxVertexCount;
}

auto MeshletGeometryResource::getMeshletMaxTriangleCount() const -> uint32_t
{
    return m_meshletMaxTriangleCount;
}
} // namespace aph
