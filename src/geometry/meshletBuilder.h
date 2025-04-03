#pragma once

#include "geometry.h"
#include "math/boundingVolume.h"

namespace aph
{

// Helper class for building meshlets from raw mesh data
class MeshletBuilder
{
public:
    MeshletBuilder() = default;
    ~MeshletBuilder() = default;
    
    // Add mesh data to be processed into meshlets
    void addMesh(const float* positions, uint32_t positionStride, uint32_t vertexCount, 
                 const uint32_t* indices, uint32_t indexCount,
                 const float* normals = nullptr, uint32_t normalStride = 0);
    
    // Build meshlets with the specified parameters
    void build(uint32_t maxVertsPerMeshlet = 64, uint32_t maxPrimsPerMeshlet = 124, 
               bool optimizeForOverdraw = true, bool optimizeForVertexFetch = true);
                
    // Access resulting data
    const std::vector<Meshlet>& getMeshlets() const { return m_meshlets; }
    const std::vector<uint32_t>& getMeshletVertices() const { return m_meshletVertices; }
    const std::vector<uint32_t>& getMeshletIndices() const { return m_meshletIndices; }
    
    // Export meshlet data to buffers ready for GPU
    void exportMeshletData(
        std::vector<Meshlet>& meshlets,
        std::vector<uint32_t>& meshletVertices,
        std::vector<uint32_t>& meshletIndices) const;
    
    // Generate submeshes from meshlets (useful for material grouping)
    std::vector<Submesh> generateSubmeshes(
        uint32_t materialIndex = 0,
        uint32_t maxMeshletsPerSubmesh = 0) const;
                                       
private:
    // Input mesh data
    struct MeshData
    {
        std::vector<float> positions;         // XYZ position data
        std::vector<float> normals;           // Optional normal data (for cone culling)
        std::vector<uint32_t> indices;        // Triangle indices
    };
    
    // Generate bounding information for a meshlet
    void computeMeshletBounds(Meshlet& meshlet);
    
    // Build cone for backface culling
    void computeMeshletCone(Meshlet& meshlet);
    
    // Group meshlets into submeshes
    void groupMeshlets();

private:
    // Input data
    MeshData m_meshData;
    
    // Processed meshlet data
    std::vector<Meshlet> m_meshlets;
    std::vector<uint32_t> m_meshletVertices;
    std::vector<uint32_t> m_meshletIndices;
    
    // Build parameters
    uint32_t m_maxVertsPerMeshlet = 64;
    uint32_t m_maxPrimsPerMeshlet = 124;
};

} // namespace aph
