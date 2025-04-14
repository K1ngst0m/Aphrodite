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

void VertexGeometryResource::bind(vk::CommandBuffer* cmdBuffer)
{
    APH_PROFILER_SCOPE();

    // Bind the vertex buffers (position and attribute buffers)
    SmallVector<vk::Buffer*> vertexBuffers;
    SmallVector<uint32_t> vertexOffsets;

    // Add position buffer (assumed to be binding 0)
    if (m_gpuData.pPositionBuffer)
    {
        vertexBuffers.push_back(m_gpuData.pPositionBuffer);
        vertexOffsets.push_back(0);
    }

    // Add attribute buffer (assumed to be binding 1)
    if (m_gpuData.pAttributeBuffer)
    {
        vertexBuffers.push_back(m_gpuData.pAttributeBuffer);
        vertexOffsets.push_back(0);
    }

    // Bind each vertex buffer individually
    for (uint32_t i = 0; i < vertexBuffers.size(); i++)
    {
        cmdBuffer->bindVertexBuffers(vertexBuffers[i], i, vertexOffsets[i]);
    }

    // Bind the index buffer
    if (m_gpuData.pIndexBuffer)
    {
        cmdBuffer->bindIndexBuffers(m_gpuData.pIndexBuffer, 0, m_gpuData.indexType);
    }

    // Set the topology
    cmdBuffer->setVertexInput(m_vertexInput);
    // Set the primitive topology
    switch (m_topology)
    {
    case PrimitiveTopology::PointList:
    case PrimitiveTopology::LineList:
    case PrimitiveTopology::LineStrip:
    case PrimitiveTopology::TriangleList:
    case PrimitiveTopology::TriangleStrip:
    case PrimitiveTopology::TriangleFan:
    case PrimitiveTopology::LineListWithAdjacency:
    case PrimitiveTopology::LineStripWithAdjacency:
    case PrimitiveTopology::TriangleListWithAdjacency:
    case PrimitiveTopology::TriangleStripWithAdjacency:
    case PrimitiveTopology::PatchList:
        // No need to call any specific function as the command buffer already knows
        // the topology from the pipeline state or setVertexInput
        break;
    }
}

void VertexGeometryResource::draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex, uint32_t instanceCount)
{
    APH_PROFILER_SCOPE();

    // Ensure submesh index is valid
    if (submeshIndex >= m_submeshes.size())
    {
        APH_ASSERT(false, "Invalid submesh index");
        return;
    }

    const Submesh& submesh = m_submeshes[submeshIndex];

    // Calculate start and count for this submesh
    // Note: This assumes meshlets are laid out contiguously in the index buffer
    uint32_t indexStart = 0;
    uint32_t indexCount = 0;

    // Iterate over meshlets in this submesh to get total index count
    for (uint32_t i = 0; i < submesh.meshletCount; ++i)
    {
        const uint32_t meshletIdx = submesh.meshletOffset + i;
        if (meshletIdx < m_gpuData.meshletCount)
        {
            // We're using a local copy because the GPU buffer layout might be different
            const Meshlet& meshlet = reinterpret_cast<const Meshlet*>(m_gpuData.pMeshletBuffer)[meshletIdx];
            if (i == 0)
            {
                // First meshlet determines the start index
                indexStart = meshlet.triangleOffset * 3;
            }
            indexCount += meshlet.triangleCount * 3;
        }
    }

    // If we have indices and draw commands, draw the submesh
    if (indexCount > 0 && m_gpuData.pIndexBuffer)
    {
        cmdBuffer->drawIndexed(DrawIndexArguments{ .indexCount    = indexCount,
                                                   .instanceCount = instanceCount,
                                                   .firstIndex    = indexStart,
                                                   .vertexOffset  = 0,
                                                   .firstInstance = 0 });
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

void MeshletGeometryResource::bind(vk::CommandBuffer* cmdBuffer)
{
    APH_PROFILER_SCOPE();

    // For the mesh shader path, we don't bind vertex/index buffers directly
    // Instead, we'll access them through storage buffers in the mesh shader

    // Set up push constants for meshlet data if needed
    // This would include buffer addresses and information needed by the mesh shader
    struct MeshletPushConstants
    {
        uint32_t meshletOffset;
        uint32_t meshletMaxVertexCount;
        uint32_t meshletMaxTriangleCount;
        uint32_t padding;
    };

    MeshletPushConstants constants{};
    constants.meshletOffset           = 0; // Will be updated for each submesh in draw()
    constants.meshletMaxVertexCount   = m_meshletMaxVertexCount;
    constants.meshletMaxTriangleCount = m_meshletMaxTriangleCount;
    constants.padding                 = 0;

    // Push these constants for the mesh shader to access
    // We'll update meshletOffset in the draw call
    Range range = { .offset = 0, .size = sizeof(MeshletPushConstants) };
    cmdBuffer->pushConstant(&constants, range);
}

void MeshletGeometryResource::draw(vk::CommandBuffer* cmdBuffer, uint32_t submeshIndex, uint32_t instanceCount)
{
    APH_PROFILER_SCOPE();

    // Check device supports mesh shading
    if (!m_pDevice->getPhysicalDevice()->getProperties().feature.meshShading)
    {
        APH_ASSERT(false, "Device does not support mesh shading");
        return;
    }

    // Ensure submesh index is valid
    if (submeshIndex >= m_submeshes.size())
    {
        APH_ASSERT(false, "Invalid submesh index");
        return;
    }

    const Submesh& submesh = m_submeshes[submeshIndex];

    // Update the meshlet offset in push constants
    struct MeshletPushConstants
    {
        uint32_t meshletOffset;
        uint32_t meshletMaxVertexCount;
        uint32_t meshletMaxTriangleCount;
        uint32_t padding;
    };

    MeshletPushConstants constants{};
    constants.meshletOffset           = submesh.meshletOffset;
    constants.meshletMaxVertexCount   = m_meshletMaxVertexCount;
    constants.meshletMaxTriangleCount = m_meshletMaxTriangleCount;
    constants.padding                 = 0;

    // Update push constants with the current submesh's meshlet offset
    Range range = { .offset = 0, .size = sizeof(MeshletPushConstants) };
    cmdBuffer->pushConstant(&constants, range);

    // Calculate meshlet workgroups: each meshlet becomes a workgroup for the mesh shader
    const uint32_t workgroupCount = submesh.meshletCount;

    if (workgroupCount > 0)
    {
        // Use dispatch instead of drawMeshTasks since it might not be directly exposed
        // This will work if the shader system maps mesh shader dispatches to the appropriate
        // native calls under the hood
        DispatchArguments args{};
        args.x = workgroupCount;
        args.y = instanceCount;
        args.z = 1;

        // Submit as a dispatch with workgroup count = meshlet count
        cmdBuffer->dispatch(args);
    }
}

//
// GeometryResourceFactory implementation
//
std::unique_ptr<IGeometryResource> GeometryResourceFactory::createGeometryResource(
    vk::Device* pDevice, const GeometryGpuData& gpuData, const std::vector<Submesh>& submeshes,
    const VertexInput& vertexInput, bool preferMeshShading)
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

} // namespace aph
