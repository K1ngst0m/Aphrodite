#include "meshletBuilder.h"

#include "common/profiler.h"

#include <meshoptimizer.h>

namespace aph
{

void MeshletBuilder::addMesh(const float* positions, uint32_t positionStride, uint32_t vertexCount,
                             const uint32_t* indices, uint32_t indexCount, const float* normals, uint32_t normalStride)
{
    APH_PROFILER_SCOPE();
    APH_ASSERT(positions && indices);
    APH_ASSERT(indexCount % 3 == 0); // Must be triangles

    // Reserve space for new data
    size_t baseIndex = m_meshData.positions.size() / 3;
    m_meshData.positions.reserve(m_meshData.positions.size() + (vertexCount * 3));
    m_meshData.indices.reserve(m_meshData.indices.size() + indexCount);

    // Copy position data
    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        const auto* pos =
            reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(positions) + (i * positionStride));
        m_meshData.positions.push_back(pos[0]);
        m_meshData.positions.push_back(pos[1]);
        m_meshData.positions.push_back(pos[2]);
    }

    // Copy normal data if available
    if (normals && normalStride > 0)
    {
        m_meshData.normals.reserve(m_meshData.normals.size() + (vertexCount * 3));
        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            const auto* normal =
                reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(normals) + (i * normalStride));
            m_meshData.normals.push_back(normal[0]);
            m_meshData.normals.push_back(normal[1]);
            m_meshData.normals.push_back(normal[2]);
        }
    }

    // Copy and adjust index data
    for (uint32_t i = 0; i < indexCount; ++i)
    {
        m_meshData.indices.push_back(indices[i] + static_cast<uint32_t>(baseIndex));
    }
}

void MeshletBuilder::build(uint32_t maxVertsPerMeshlet, uint32_t maxPrimsPerMeshlet, bool optimizeForOverdraw,
                           bool optimizeForVertexFetch)
{
    APH_PROFILER_SCOPE();

    if (m_meshData.positions.empty() || m_meshData.indices.empty())
    {
        return;
    }

    m_maxVertsPerMeshlet = maxVertsPerMeshlet;
    m_maxPrimsPerMeshlet = maxPrimsPerMeshlet;

    // Get vertex and index data
    const float* positions   = m_meshData.positions.data();
    const uint32_t* indices  = m_meshData.indices.data();
    const size_t indexCount  = m_meshData.indices.size();
    const size_t vertexCount = m_meshData.positions.size() / 3;

    // Step 1: Optimize mesh if requested
    std::vector<uint32_t> optimizedIndices;
    optimizedIndices.resize(indexCount);
    std::memcpy(optimizedIndices.data(), indices, indexCount * sizeof(uint32_t));

    if (optimizeForOverdraw)
    {
        // Cluster triangles for overdraw optimization
        meshopt_optimizeOverdraw(optimizedIndices.data(), optimizedIndices.data(), indexCount, positions, vertexCount,
                                 sizeof(float) * 3,
                                 1.05f); // Threshold
    }

    // Step 2: Generate meshlets
    // First calculate the maximum number of meshlets we might need
    const size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, maxVertsPerMeshlet, maxPrimsPerMeshlet);

    // Allocate temporary storage for meshlets and meshlet data
    std::vector<meshopt_Meshlet> meshletData(maxMeshlets);
    std::vector<unsigned int> meshletVertices(maxMeshlets * maxVertsPerMeshlet);
    std::vector<unsigned char> meshletTriangles(maxMeshlets * maxPrimsPerMeshlet * 3);

    // Build meshlets
    size_t meshletCount = meshopt_buildMeshlets(meshletData.data(), meshletVertices.data(), meshletTriangles.data(),
                                                optimizedIndices.data(), indexCount, positions, vertexCount,
                                                sizeof(float) * 3, maxVertsPerMeshlet, maxPrimsPerMeshlet,
                                                0.f // cone_weight (0 means no cone culling optimization)
    );

    // Step 3: Convert meshoptimizer meshlets into our format

    // Reserve space for our meshlet data structures
    m_meshlets.clear();
    m_meshletVertices.clear();
    m_meshletIndices.clear();

    m_meshlets.reserve(meshletCount);

    for (size_t i = 0; i < meshletCount; ++i)
    {
        const meshopt_Meshlet& meshlet = meshletData[i];

        // Skip degenerate meshlets
        if (meshlet.triangle_count == 0)
        {
            continue;
        }

        Meshlet ourMeshlet;
        ourMeshlet.vertexCount    = meshlet.vertex_count;
        ourMeshlet.triangleCount  = meshlet.triangle_count;
        ourMeshlet.vertexOffset   = static_cast<uint32_t>(m_meshletVertices.size());
        ourMeshlet.triangleOffset = static_cast<uint32_t>(m_meshletIndices.size() / 3);
        ourMeshlet.materialIndex  = 0; // Default material index

        // Copy vertex indices
        for (uint32_t j = 0; j < meshlet.vertex_count; ++j)
        {
            m_meshletVertices.push_back(meshletVertices[meshlet.vertex_offset + j]);
        }

        // Copy triangle indices (converting from bytes to uint32_t)
        for (uint32_t j = 0; j < meshlet.triangle_count * 3; j += 3)
        {
            unsigned char a = meshletTriangles[meshlet.triangle_offset + j + 0];
            unsigned char b = meshletTriangles[meshlet.triangle_offset + j + 1];
            unsigned char c = meshletTriangles[meshlet.triangle_offset + j + 2];

            m_meshletIndices.push_back(a);
            m_meshletIndices.push_back(b);
            m_meshletIndices.push_back(c);
        }

        // Compute bounds and cone data for the meshlet
        computeMeshletBounds(ourMeshlet);
        computeMeshletCone(ourMeshlet);

        m_meshlets.push_back(ourMeshlet);
    }

    // Step 4: If requested, optimize for vertex fetch
    if (optimizeForVertexFetch && !m_meshlets.empty())
    {
        // This is a more complex optimization that we might implement in the future
        // For now, we'll rely on the meshlet generation to give us reasonably good cache behavior
    }
}

void MeshletBuilder::computeMeshletBounds(Meshlet& meshlet)
{
    // Compute bounding sphere for the meshlet
    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float minZ = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;
    float maxZ = -FLT_MAX;

    // Iterate over all vertices in the meshlet
    for (uint32_t i = 0; i < meshlet.vertexCount; ++i)
    {
        uint32_t index = m_meshletVertices[meshlet.vertexOffset + i];
        if (index < m_meshData.positions.size() / 3)
        {
            float x = m_meshData.positions[(index * 3) + 0];
            float y = m_meshData.positions[(index * 3) + 1];
            float z = m_meshData.positions[(index * 3) + 2];

            minX = std::min(minX, x);
            minY = std::min(minY, y);
            minZ = std::min(minZ, z);

            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
            maxZ = std::max(maxZ, z);
        }
    }

    // Calculate center and radius
    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;

    // For radius, find the most distant vertex from center
    float radius = 0.0f;
    for (uint32_t i = 0; i < meshlet.vertexCount; ++i)
    {
        uint32_t index = m_meshletVertices[meshlet.vertexOffset + i];
        if (index < m_meshData.positions.size() / 3)
        {
            float x = m_meshData.positions[(index * 3) + 0] - centerX;
            float y = m_meshData.positions[(index * 3) + 1] - centerY;
            float z = m_meshData.positions[(index * 3) + 2] - centerZ;

            float distSq = (y * y) + (x * x) + (z * z);
            radius       = std::max(radius, std::sqrt(distSq));
        }
    }

    // Set bounding sphere
    meshlet.positionBounds[0] = centerX;
    meshlet.positionBounds[1] = centerY;
    meshlet.positionBounds[2] = centerZ;
    meshlet.positionBounds[3] = radius;
}

void MeshletBuilder::computeMeshletCone(Meshlet& meshlet)
{
    // Compute view cone for backface culling
    float coneX     = 0.0f;
    float coneY     = 0.0f;
    float coneZ     = 0.0f;
    float coneAngle = 0.0f;

    // We need normals for this
    if (m_meshData.normals.empty())
    {
        // No usable data for cone culling, use default values
        meshlet.coneCenterAndAngle[0] = 0.0f;
        meshlet.coneCenterAndAngle[1] = 0.0f;
        meshlet.coneCenterAndAngle[2] = 1.0f;
        meshlet.coneCenterAndAngle[3] = 0.0f; // No culling (angle = 0)
        return;
    }

    // Average the normals of all triangles in the meshlet
    for (uint32_t i = 0; i < meshlet.triangleCount; ++i)
    {
        uint32_t idx0 = m_meshletIndices[((meshlet.triangleOffset + i) * 3) + 0];
        uint32_t idx1 = m_meshletIndices[((meshlet.triangleOffset + i) * 3) + 1];
        uint32_t idx2 = m_meshletIndices[((meshlet.triangleOffset + i) * 3) + 2];

        idx0 = m_meshletVertices[meshlet.vertexOffset + idx0];
        idx1 = m_meshletVertices[meshlet.vertexOffset + idx1];
        idx2 = m_meshletVertices[meshlet.vertexOffset + idx2];

        if (idx0 < m_meshData.normals.size() / 3 && idx1 < m_meshData.normals.size() / 3 &&
            idx2 < m_meshData.normals.size() / 3)
        {
            // Average the normals for this triangle
            float nx = (m_meshData.normals[(idx0 * 3) + 0] + m_meshData.normals[(idx1 * 3) + 0] +
                        m_meshData.normals[(idx2 * 3) + 0]) /
                       3.0f;

            float ny = (m_meshData.normals[(idx0 * 3) + 1] + m_meshData.normals[(idx1 * 3) + 1] +
                        m_meshData.normals[(idx2 * 3) + 1]) /
                       3.0f;

            float nz = (m_meshData.normals[(idx0 * 3) + 2] + m_meshData.normals[(idx1 * 3) + 2] +
                        m_meshData.normals[(idx2 * 3) + 2]) /
                       3.0f;

            // Normalize
            float len = std::sqrt((nx * nx) + (ny * ny) + (nz * nz));
            if (len > 0.0f)
            {
                nx /= len;
                ny /= len;
                nz /= len;
            }

            // Accumulate
            coneX += nx;
            coneY += ny;
            coneZ += nz;
        }
    }

    // Normalize the average normal
    float len = std::sqrt((coneX * coneX) + (coneY * coneY) + (coneZ * coneZ));
    if (len > 0.0f)
    {
        coneX /= len;
        coneY /= len;
        coneZ /= len;
    }
    else
    {
        // If we couldn't compute a valid cone, use a default
        coneX = 0.0f;
        coneY = 0.0f;
        coneZ = 1.0f;
    }

    // Calculate cone angle as the maximum angle between average and any triangle normal
    for (uint32_t i = 0; i < meshlet.triangleCount; ++i)
    {
        uint32_t idx0 = m_meshletIndices[((meshlet.triangleOffset + i) * 3) + 0];
        uint32_t idx1 = m_meshletIndices[((meshlet.triangleOffset + i) * 3) + 1];
        uint32_t idx2 = m_meshletIndices[((meshlet.triangleOffset + i) * 3) + 2];

        idx0 = m_meshletVertices[meshlet.vertexOffset + idx0];
        idx1 = m_meshletVertices[meshlet.vertexOffset + idx1];
        idx2 = m_meshletVertices[meshlet.vertexOffset + idx2];

        if (idx0 < m_meshData.normals.size() / 3 && idx1 < m_meshData.normals.size() / 3 &&
            idx2 < m_meshData.normals.size() / 3)
        {
            // Average the normals for this triangle
            float nx = (m_meshData.normals[idx0 * 3 + 0] + m_meshData.normals[idx1 * 3 + 0] +
                        m_meshData.normals[idx2 * 3 + 0]) /
                       3.0f;

            float ny = (m_meshData.normals[idx0 * 3 + 1] + m_meshData.normals[idx1 * 3 + 1] +
                        m_meshData.normals[idx2 * 3 + 1]) /
                       3.0f;

            float nz = (m_meshData.normals[idx0 * 3 + 2] + m_meshData.normals[idx1 * 3 + 2] +
                        m_meshData.normals[idx2 * 3 + 2]) /
                       3.0f;

            // Normalize
            float tlen = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (tlen > 0.0f)
            {
                nx /= tlen;
                ny /= tlen;
                nz /= tlen;
            }

            // Calculate dot product to get angle
            float dot   = nx * coneX + ny * coneY + nz * coneZ;
            float angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));

            coneAngle = std::max(coneAngle, angle);
        }
    }

    // Store cone center and angle
    meshlet.coneCenterAndAngle[0] = coneX;
    meshlet.coneCenterAndAngle[1] = coneY;
    meshlet.coneCenterAndAngle[2] = coneZ;
    meshlet.coneCenterAndAngle[3] = coneAngle;
}

void MeshletBuilder::exportMeshletData(std::vector<Meshlet>& meshlets, std::vector<uint32_t>& meshletVertices,
                                       std::vector<uint32_t>& meshletIndices) const
{
    meshlets        = m_meshlets;
    meshletVertices = m_meshletVertices;
    meshletIndices  = m_meshletIndices;
}

auto MeshletBuilder::generateSubmeshes(uint32_t materialIndex, uint32_t maxMeshletsPerSubmesh) const
    -> std::vector<Submesh>
{
    std::vector<Submesh> submeshes;

    // Handle empty case
    if (m_meshlets.empty())
    {
        return submeshes;
    }

    // If we're not partitioning by count, just create one submesh for all meshlets
    if (maxMeshletsPerSubmesh == 0 || maxMeshletsPerSubmesh >= m_meshlets.size())
    {
        Submesh submesh;
        submesh.meshletOffset = 0;
        submesh.meshletCount  = static_cast<uint32_t>(m_meshlets.size());
        submesh.materialIndex = materialIndex;

        // Calculate bounds
        BoundingBox bounds;
        bounds.min = glm::vec3(FLT_MAX);
        bounds.max = glm::vec3(-FLT_MAX);

        for (const auto& meshlet : m_meshlets)
        {
            // Extend bounds by meshlet sphere
            float radius = meshlet.positionBounds[3];
            glm::vec3 center(meshlet.positionBounds[0], meshlet.positionBounds[1], meshlet.positionBounds[2]);

            bounds.min = glm::min(bounds.min, center - glm::vec3(radius));
            bounds.max = glm::max(bounds.max, center + glm::vec3(radius));
        }

        submesh.bounds = bounds;
        submeshes.push_back(submesh);
        return submeshes;
    }

    // Otherwise, partition meshlets into multiple submeshes
    uint32_t meshletCount = static_cast<uint32_t>(m_meshlets.size());
    uint32_t submeshCount = (meshletCount + maxMeshletsPerSubmesh - 1) / maxMeshletsPerSubmesh;

    for (uint32_t i = 0; i < submeshCount; ++i)
    {
        Submesh submesh;
        submesh.meshletOffset = i * maxMeshletsPerSubmesh;
        submesh.meshletCount  = std::min(maxMeshletsPerSubmesh, meshletCount - submesh.meshletOffset);
        submesh.materialIndex = materialIndex;

        // Calculate bounds for this submesh
        BoundingBox bounds;
        bounds.min = glm::vec3(FLT_MAX);
        bounds.max = glm::vec3(-FLT_MAX);

        for (uint32_t j = 0; j < submesh.meshletCount; ++j)
        {
            const auto& meshlet = m_meshlets[submesh.meshletOffset + j];

            // Extend bounds by meshlet sphere
            float radius = meshlet.positionBounds[3];
            glm::vec3 center(meshlet.positionBounds[0], meshlet.positionBounds[1], meshlet.positionBounds[2]);

            bounds.min = glm::min(bounds.min, center - glm::vec3(radius));
            bounds.max = glm::max(bounds.max, center + glm::vec3(radius));
        }

        submesh.bounds = bounds;
        submeshes.push_back(submesh);
    }

    return submeshes;
}

auto MeshletBuilder::getMeshlets() const -> const std::vector<Meshlet>&
{
    return m_meshlets;
}

auto MeshletBuilder::getMeshletVertices() const -> const std::vector<uint32_t>&
{
    return m_meshletVertices;
}

auto MeshletBuilder::getMeshletIndices() const -> const std::vector<uint32_t>&
{
    return m_meshletIndices;
}
} // namespace aph
