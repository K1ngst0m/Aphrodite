struct VertexIn
{
    float4 position;
    float2 uv;
    float2 padding;
};

struct TransformMatrix
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    float4 transform(float4 position)
    {
        float4x4 mvp = mul(mul(model, view), projection);
        return mul(position, mvp);
    }
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct Meshlet
{
    uint vertexCount;
    uint triangleCount;
    uint vertexOffset;
    uint triangleOffset;
    float4 positionBounds;
    float4 coneCenterAndAngle;
    uint materialIndex;
};

// Maximum limits for meshlet processing
#define MAX_MESHLET_VERTICES 64
#define MAX_MESHLET_TRIANGLES 124
#define MAX_MESH_WORK_GROUP_SIZE 32

// New buffer types for meshlet data
struct MeshletVertex
{
    uint vertexIndex;
};

// ╭────────────────────────────────────────────╮
// │                Task Shader                 │
// ╰────────────────────────────────────────────╯
struct TaskPayload
{
    uint meshletCount;
    uint baseID;
};

groupshared TaskPayload taskPayload;

// Function to count the number of meshlets
uint GetMeshletCount()
{
    // For a proper implementation, we would get this from a resource
    // Currently we just probe for the first meshlet to verify it's valid
    int count = 0;
    
    // Use a hardcoded limit for safety
    const int MAX_POSSIBLE_MESHLETS = 64;
    
    for (int i = 0; i < MAX_POSSIBLE_MESHLETS; i++)
    {
        // Attempt to get meshlet
        Meshlet meshlet = handle::meshlets_cube.get<Meshlet>(i);
        
        // If meshlet has vertices, it's valid
        if (meshlet.vertexCount > 0)
        {
            count++;
        }
        else
        {
            // No more valid meshlets
            break;
        }
    }
    
    return count;
}

[numthreads(1, 1, 1)]
[shader("amplification")]
void taskMain()
{
    // Get the number of meshlets from the buffer
    taskPayload.meshletCount = GetMeshletCount();
    taskPayload.baseID = 0;
    
    // Dispatch one mesh shader invocation per meshlet
    DispatchMesh(taskPayload.meshletCount, 1, 1, taskPayload);
}

// ╭────────────────────────────────────────────╮
// │                Mesh Shader                 │
// ╰────────────────────────────────────────────╯
VertexIn GetVertex(uint vertexIndex)
{
    // Create a vertex from separated position and attribute buffers
    VertexIn vertex;
    
    // Get position from position buffer
    vertex.position = handle::position_cube.get<float4>(vertexIndex);
    
    // Get UV from attribute buffer
    vertex.uv = handle::attribute_cube.get<float2>(vertexIndex);
    
    // Set padding to zero
    vertex.padding = float2(0, 0);
    
    return vertex;
}

Meshlet GetMeshlet(uint meshletId)
{
    // Get meshlet data from the bindless buffer
    return handle::meshlets_cube.get<Meshlet>(meshletId);
}

uint GetMeshletVertexIndex(uint meshletVertexIndex)
{
    // Get the global vertex index for this meshlet vertex
    return handle::meshlet_vertices.get<uint>(meshletVertexIndex);
}

uint3 GetMeshletTriangle(uint meshletTriangleIndex)
{
    // Get the indices for this triangle
    uint i0 = handle::meshlet_indices.get<uint>(meshletTriangleIndex * 3 + 0);
    uint i1 = handle::meshlet_indices.get<uint>(meshletTriangleIndex * 3 + 1);
    uint i2 = handle::meshlet_indices.get<uint>(meshletTriangleIndex * 3 + 2);
    return uint3(i0, i1, i2);
}

[outputtopology("triangle")]
[numthreads(MAX_MESH_WORK_GROUP_SIZE, 1, 1)]
[shader("mesh")]
void meshMain(
    in TaskPayload payload,
    out indices uint3 triangles[MAX_MESHLET_TRIANGLES], 
    out vertices VertexOutput vertices[MAX_MESHLET_VERTICES], 
    uint3 tid : SV_GroupThreadID,
    uint3 gid : SV_GroupID)
{
    // Get the meshlet for this group invocation
    uint meshletId = gid.x;
    Meshlet meshlet = GetMeshlet(meshletId);
    
    // Set output count based on the meshlet
    SetMeshOutputCounts(meshlet.vertexCount, meshlet.triangleCount);
    
    // Thread 0 processes vertices for this meshlet
    if (tid.x == 0)
    {
        // Get transform matrix through bindless handle
        let mat = handle::transform_cube.get<TransformMatrix>();
        
        // Process all vertices in the meshlet
        for (uint i = 0; i < meshlet.vertexCount; i++)
        {
            // Get global vertex index from meshlet vertices buffer
            uint vertexId = GetMeshletVertexIndex(meshlet.vertexOffset + i);
            VertexIn vertex = GetVertex(vertexId);
            
            // Transform and assign vertex data
            vertices[i].position = mat.transform(vertex.position);
            vertices[i].uv = vertex.uv;
        }
    }
    
    // Distribute triangle processing among threads
    if (tid.x < meshlet.triangleCount)
    {
        uint triangleId = tid.x;
        
        // Get triangle indices from meshlet index buffer (already local to the meshlet)
        uint3 indices = GetMeshletTriangle(meshlet.triangleOffset + triangleId);
        
        // Assign to output
        triangles[triangleId] = indices;
    }
}

// ╭────────────────────────────────────────────╮
// │               Fragment Shader              │
// ╰────────────────────────────────────────────╯
[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_TARGET
{
    let color = handle::texture_container.sample(handle::samp, input.uv);
    return color;
}
