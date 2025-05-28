import modules.meshlet;
import modules.math;
import modules.bindless;

struct HandleData
{
uint texture_container;
uint samp;
uint transform_cube;
uint position_cube;
uint attribute_cube;
uint index_cube;
uint meshlets_cube;
uint meshlet_vertices;
uint meshlet_indices;
uint mesh_metadata;
uint submeshes_cube;
};
[[vk::binding(0, Set::eHandle)]] ConstantBuffer<HandleData, Std430DataLayout> handleData;

// Define maximum limits for meshlet processing
#define MAX_MESHLET_VERTICES 64
#define MAX_MESHLET_TRIANGLES 124
#define MAX_MESH_WORK_GROUP_SIZE 32

struct TransformMatrix
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

// Output vertex structure to match the module's VertexOutput
struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

// ╭────────────────────────────────────────────╮
// │                Task Shader                 │
// ╰────────────────────────────────────────────╯
groupshared meshlet::TaskPayload taskPayload;

// Create MeshProcessor from HandleData
meshlet::MeshProcessor createMeshProcessor()
{
    // Create a MeshProcessor directly from individual handle IDs
    return meshlet::MeshProcessor.create(
        bindless::Buffer(handleData.meshlets_cube),
        bindless::Buffer(handleData.meshlet_vertices),
        bindless::Buffer(handleData.meshlet_indices),
        bindless::Buffer(handleData.submeshes_cube),
        bindless::Buffer(handleData.mesh_metadata),
        bindless::Buffer(handleData.position_cube),
        bindless::Buffer(handleData.attribute_cube),
        bindless::Buffer(handleData.index_cube),
        bindless::Buffer(handleData.transform_cube)
    );
}

// Get MVP matrix from transform handle
float4x4 getMVPMatrix()
{
    TransformMatrix transform = bindless::getBuffer<TransformMatrix>(handleData.transform_cube);
    return mul(mul(transform.model, transform.view), transform.projection);
}

[numthreads(1, 1, 1)]
[shader("amplification")]
void taskMain(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint submeshIndex = DispatchThreadID.x;
    
    // Create mesh processor
    meshlet::MeshProcessor processor = createMeshProcessor();
    
    // Create task payload using the processor
    taskPayload = processor.createTaskPayload(submeshIndex);
    
    // Dispatch mesh shader invocations
    DispatchMesh(taskPayload.meshletCount, 1, 1, taskPayload);
}

// ╭────────────────────────────────────────────╮
// │                Mesh Shader                 │
// ╰────────────────────────────────────────────╯
[outputtopology("triangle")]
[numthreads(MAX_MESH_WORK_GROUP_SIZE, 1, 1)]
[shader("mesh")]
void meshMain(
    in meshlet::TaskPayload payload,
    out indices uint3 triangles[MAX_MESHLET_TRIANGLES], 
    out vertices VertexOutput vertices[MAX_MESHLET_VERTICES], 
    uint3 tid : SV_GroupThreadID,
    uint3 gid : SV_GroupID)
{
    // Create mesh processor
    meshlet::MeshProcessor processor = createMeshProcessor();
    
    // Get meshlet ID and data
    uint meshletId = processor.getMeshletId(payload, gid.x);
    meshlet::Meshlet meshlet = processor.getMeshlet(meshletId);
    
    // Set output counts based on the meshlet
    SetMeshOutputCounts(meshlet.vertexCount, meshlet.triangleCount);
    
    // Get transformation matrix
    float4x4 mvpMatrix = getMVPMatrix();

    // Thread 0 processes vertices
    if (tid.x == 0)
    {
        for (uint i = 0; i < meshlet.vertexCount; i++)
        {
            uint vertexId = processor.getVertexIndex(meshlet.vertexOffset + i);
            float3 position = processor.getPosition(vertexId);
            float2 uv = processor.getUV(vertexId);
            
            vertices[i].position = mul(float4(position, 1.0), mvpMatrix);
            vertices[i].uv = uv;
        }
    }
    
    // Process triangles using the processor
    if (tid.x < meshlet.triangleCount)
    {
        uint meshletTriangleIndex = meshlet.triangleOffset + tid.x;
        uint3 indices = processor.getTriangle(meshletTriangleIndex);
        triangles[tid.x] = indices;
    }
}

// ╭────────────────────────────────────────────╮
// │               Fragment Shader              │
// ╰────────────────────────────────────────────╯
[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_TARGET
{
    // Sample texture using bindless functions directly
    Texture2D tex = bindless::getImage(handleData.texture_container);
    SamplerState samp = bindless::getSampler(handleData.samp);
    return tex.Sample(samp, input.uv);
}
