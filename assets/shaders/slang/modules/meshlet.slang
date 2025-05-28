// meshlet.slang - Mesh shading utility functions and standardized access patterns

// Import other modules with proper module syntax
import bindless;
import math;
import transform;

namespace meshlet
{
    //==========================================
    // Core Meshlet Data Structures 
    //==========================================
    
    // Standard meshlet structure that matches the C++ side
    struct Meshlet
    {
        uint vertexCount;    // Number of vertices used by this meshlet
        uint triangleCount;  // Number of triangles in this meshlet
        uint vertexOffset;   // Offset into meshlet vertex array
        uint triangleOffset; // Offset into meshlet triangle array
        float4 positionBounds;     // Bounding sphere: xyz = center, w = radius
        float4 coneCenterAndAngle; // xyz = cone center, w = cone cutoff angle
        uint materialIndex;  // Material index for this meshlet
    };
    
    // Submesh structure that matches the C++ side
    struct Submesh
    {
        uint meshletOffset;  // First meshlet index in the mesh
        uint meshletCount;   // Number of meshlets in this submesh
        uint materialIndex;  // Material index used by this submesh
        uint padding0;       // Padding to maintain alignment
        float4 boundsMin;    // AABB minimum bounds
        float4 boundsMax;    // AABB maximum bounds 
    };
    
    // Mesh metadata structure
    struct MeshMetadata
    {
        uint meshletCount;   // Total number of meshlets
        uint vertexCount;    // Total number of vertices
        uint indexCount;     // Total number of indices
        uint submeshCount;   // Total number of submeshes
    };
    
    // Common task shader payload structure
    struct TaskPayload
    {
        uint meshletCount;   // Number of meshlets to process
        uint meshletOffset;  // Starting meshlet index
        uint materialIndex;  // Material index
    };
    
    // Common mesh shader constants
    static const uint MAX_MESHLET_VERTICES = 64;
    static const uint MAX_MESHLET_TRIANGLES = 124;
    static const uint MAX_MESH_WORK_GROUP_SIZE = 32;
    
    //==========================================
    // Handle Access Utilities 
    //==========================================
    
    // Structure to hold standard mesh resource handles
    // This should be initialized at the shader level using handles provided by the engine
    struct MeshResourceHandles
    {
        // Meshlet structural data
        bindless::Buffer meshlets;        // Meshlet definitions
        bindless::Buffer meshletVertices; // Local-to-global vertex indices 
        bindless::Buffer meshletIndices;  // Triangle indices (local to meshlet)
        bindless::Buffer submeshes;       // Submesh definitions
        bindless::Buffer metadata;        // Mesh metadata
        
        // Vertex attribute data
        bindless::Buffer positions;       // Vertex position buffer
        bindless::Buffer attributes;      // Vertex attributes buffer (UVs, etc.)
        bindless::Buffer indices;         // Optional global index buffer
        
        // Transform data
        bindless::Buffer transform;       // Transform matrix buffer
    };
    
    //==========================================
    // Meshlet Data Access Functions 
    //==========================================
    
    // Get mesh metadata
    MeshMetadata getMeshMetadata(MeshResourceHandles handles)
    {
        return handles.metadata.get<MeshMetadata>();
    }
    
    // Get submesh count (convenience function)
    uint getSubmeshCount(MeshResourceHandles handles)
    {
        return getMeshMetadata(handles).submeshCount;
    }
    
    // Get meshlet count (convenience function)
    uint getMeshletCount(MeshResourceHandles handles)
    {
        return getMeshMetadata(handles).meshletCount;
    }
    
    // Get submesh by index
    Submesh getSubmesh(MeshResourceHandles handles, uint submeshIndex)
    {
        return handles.submeshes.get<Submesh>(submeshIndex);
    }
    
    // Get meshlet by index
    Meshlet getMeshlet(MeshResourceHandles handles, uint meshletIndex)
    {
        return handles.meshlets.get<Meshlet>(meshletIndex);
    }
    
    // Get vertex position
    float3 getVertexPosition(MeshResourceHandles handles, uint vertexIndex)
    {
        return handles.positions.get<float4>(vertexIndex).xyz;
    }
    
    // Get vertex UV
    float2 getVertexUV(MeshResourceHandles handles, uint vertexIndex)
    {
        return handles.attributes.get<float2>(vertexIndex);
    }
    
    // Get meshlet vertex index
    uint getMeshletVertexIndex(MeshResourceHandles handles, uint meshletVertexIndex)
    {
        return handles.meshletVertices.get<uint>(meshletVertexIndex);
    }
    
    // Get meshlet triangle as three indices
    uint3 getMeshletTriangle(MeshResourceHandles handles, uint meshletTriangleIndex)
    {
        uint baseIndex = meshletTriangleIndex * 3;
        uint i0 = handles.meshletIndices.get<uint>(baseIndex);
        uint i1 = handles.meshletIndices.get<uint>(baseIndex + 1);
        uint i2 = handles.meshletIndices.get<uint>(baseIndex + 2);
        return uint3(i0, i1, i2);
    }
    
    //==========================================
    // Culling and Optimization Functions 
    //==========================================
    
    // Frustum culling - all planes in clip space
    bool frustumCull(Meshlet meshlet, float4 frustumPlanes[6])
    {
        float3 center = meshlet.positionBounds.xyz;
        float radius = meshlet.positionBounds.w;
        
        // Test against all frustum planes
        for (int i = 0; i < 6; i++)
        {
            float4 plane = frustumPlanes[i];
            float dist = dot(float4(center, 1.0), plane);
            if (dist < -radius)
            {
                return true; // Culled - fully outside frustum
            }
        }
        
        return false; // Not culled
    }
    
    // Backface cone culling
    bool coneCull(Meshlet meshlet, float3 viewPosition)
    {
        // Get cone data
        float3 coneCenter = meshlet.coneCenterAndAngle.xyz;
        float coneAngle = meshlet.coneCenterAndAngle.w;
        
        // Get view direction to center of the mesh
        float3 viewDir = normalize(meshlet.positionBounds.xyz - viewPosition);
        
        // Dot product between cone axis and view direction
        float dp = dot(coneCenter, viewDir);
        
        // If dp > cos(coneAngle), the entire meshlet faces away from the view
        return dp > coneAngle;
    }
    
    // Calculate depth for visibility sorting
    float calculateMeshletDepth(Meshlet meshlet, float4x4 viewProj)
    {
        // Transform meshlet center to clip space
        float4 clipPos = mul(float4(meshlet.positionBounds.xyz, 1.0), viewProj);
        
        // Return depth (Z/W in clip space)
        return clipPos.z / clipPos.w;
    }
    
    //==========================================
    // Mesh Shader Task/Amplification Utilities 
    //==========================================
    
    // Task setup utility - call from task shader to setup meshlet workload for given submesh
    TaskPayload setupSubmeshTask(MeshResourceHandles handles, uint submeshIndex)
    {
        Submesh submesh = getSubmesh(handles, submeshIndex);
        
        TaskPayload payload;
        payload.meshletCount = submesh.meshletCount;
        payload.meshletOffset = submesh.meshletOffset;
        payload.materialIndex = submesh.materialIndex;
        
        return payload;
    }
    
    // Calculate meshlet index from task payload and group ID
    uint calculateMeshletIndex(TaskPayload payload, uint groupIndex)
    {
        if (payload.meshletCount > 1) {
            // Submesh approach - calculate meshlet ID using offset + group index
            return payload.meshletOffset + groupIndex;
        } else {
            // Direct meshlet approach - use the offset directly
            return payload.meshletOffset;
        }
    }
    
    //==========================================
    // Mesh Shader Processing Utilities 
    //==========================================
    
    // Define standard vertex transform function
    struct VertexOutput
    {
        float4 position : SV_Position;
        float2 uv : TEXCOORD0;
    };
    
    // We'll use a simple non-templated version for now
    // This function implements vertex processing without templates
    void processMeshletVertices(
        MeshResourceHandles handles,
        Meshlet meshlet,
        out VertexOutput vertices[MAX_MESHLET_VERTICES],
        float4x4 transformMatrix)
    {
        // Process all vertices in the meshlet
        for (uint i = 0; i < meshlet.vertexCount; i++)
        {
            // Get global vertex index from meshlet vertices buffer
            uint vertexId = getMeshletVertexIndex(handles, meshlet.vertexOffset + i);
            
            // Get position
            float3 position = getVertexPosition(handles, vertexId);
            
            // Get UV
            float2 uv = getVertexUV(handles, vertexId);
            
            // Transform and construct output vertex
            vertices[i].position = mul(float4(position, 1.0), transformMatrix);
            vertices[i].uv = uv;
        }
    }
    
    //==========================================
    // MeshProcessor - Easy to use interface
    //==========================================

    // User-friendly mesh processor class
    struct MeshProcessor
    {
        MeshResourceHandles _handles;
        
        // Static factory method to create from individual buffers
        static MeshProcessor create(
            bindless::Buffer meshlets,
            bindless::Buffer meshletVertices,
            bindless::Buffer meshletIndices,
            bindless::Buffer submeshes,
            bindless::Buffer metadata,
            bindless::Buffer positions, 
            bindless::Buffer attributes,
            bindless::Buffer indices = {},
            bindless::Buffer transform = {})
        {
            MeshProcessor processor;
            processor._handles.meshlets = meshlets;
            processor._handles.meshletVertices = meshletVertices;
            processor._handles.meshletIndices = meshletIndices;
            processor._handles.submeshes = submeshes;
            processor._handles.metadata = metadata;
            processor._handles.positions = positions;
            processor._handles.attributes = attributes;
            processor._handles.indices = indices;
            processor._handles.transform = transform;
            return processor;
        }
        
        // Static factory method to create from HandleData
        static MeshProcessor createFromHandleData(uint[11] handleData)
        {
            MeshProcessor processor;
            processor._handles.meshlets = bindless::Buffer(handleData[6]);        // meshlets_cube
            processor._handles.meshletVertices = bindless::Buffer(handleData[7]); // meshlet_vertices
            processor._handles.meshletIndices = bindless::Buffer(handleData[8]);  // meshlet_indices
            processor._handles.submeshes = bindless::Buffer(handleData[10]);      // submeshes_cube
            processor._handles.metadata = bindless::Buffer(handleData[9]);        // mesh_metadata
            processor._handles.positions = bindless::Buffer(handleData[3]);       // position_cube
            processor._handles.attributes = bindless::Buffer(handleData[4]);      // attribute_cube
            processor._handles.indices = bindless::Buffer(handleData[5]);         // index_cube
            processor._handles.transform = bindless::Buffer(handleData[2]);       // transform_cube
            return processor;
        }
        
        // Task shader helpers
        TaskPayload createTaskPayload(uint submeshIndex)
        {
            return setupSubmeshTask(_handles, submeshIndex);
        }
        
        // Mesh shader helpers
        uint getMeshletId(TaskPayload payload, uint groupIndex)
        {
            return calculateMeshletIndex(payload, groupIndex);
        }
        
        Meshlet getMeshlet(uint meshletId)
        {
            return meshlet::getMeshlet(_handles, meshletId);
        }
        
        // Process vertices helper
        void processVertices(Meshlet meshlet, float4x4 transform, out VertexOutput vertices[MAX_MESHLET_VERTICES])
        {
            processMeshletVertices(_handles, meshlet, vertices, transform);
        }
        
        // We don't provide a direct triangle processing method to avoid output compatibility issues
        // Instead, users should process triangles directly with getTriangle in their mesh shader
        
        // Accessor functions for direct access
        uint getVertexIndex(uint meshletVertexIndex)
        {
            return getMeshletVertexIndex(_handles, meshletVertexIndex);
        }
        
        float3 getPosition(uint vertexIndex)
        {
            return getVertexPosition(_handles, vertexIndex);
        }
        
        float2 getUV(uint vertexIndex)
        {
            return getVertexUV(_handles, vertexIndex);
        }
        
        uint3 getTriangle(uint meshletTriangleIndex)
        {
            return getMeshletTriangle(_handles, meshletTriangleIndex);
        }
    };
} 