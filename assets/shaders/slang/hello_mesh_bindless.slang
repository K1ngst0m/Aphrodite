// ╭────────────────────────────────────────────╮
// │                Task Shader                 │
// ╰────────────────────────────────────────────╯
struct DummyPayLoad
{
    uint dummyData;
};

groupshared DummyPayLoad dummyPayLoad;

[numthreads(1, 1, 1)]
[shader("amplification")]
void taskMain()
{
    DispatchMesh(3, 1, 1, dummyPayLoad);
}

// ╭────────────────────────────────────────────╮
// │                Mesh Shader                 │
// ╰────────────────────────────────────────────╯
#define MAX_NUM_VERTS 36
#define MAX_NUM_PRIMS (MAX_NUM_VERTS / 3)

[[vk::binding(0, 0)]] Texture2D sampledImage[];
[[vk::binding(3, 2)]] SamplerState samp;

struct VertexIn
{
    float4 position;
    float2 uv;
    float2 padding;
};

[[vk::binding(0, 1)]] cbuffer handle
{
    uint textureId;
    TransformMatrix* uboPtr;
    VertexIn* vertexBufferPtr;
    uint* indexBufferPtr;
};

struct TransformMatrix
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[outputtopology("triangle")]
[numthreads(32, 1, 1)]
[shader("mesh")]
void meshMain(out indices uint3 triangles[MAX_NUM_PRIMS], out vertices VertexOutput vertices[MAX_NUM_VERTS], uint3 tid : SV_GroupThreadID)
{
    SetMeshOutputCounts(MAX_NUM_VERTS, MAX_NUM_PRIMS);

    if (tid.x >= 12)
    {
        return;
    }

    TransformMatrix mat = *uboPtr;
    float4x4 mvp = mul(mul(mat.model, mat.view), mat.projection);

    int i0 = tid.x * 3;

    VertexIn v0 = vertexBufferPtr[indexBufferPtr[i0]];
    VertexIn v1 = vertexBufferPtr[indexBufferPtr[i0 + 1]];
    VertexIn v2 = vertexBufferPtr[indexBufferPtr[i0 + 2]];

    vertices[i0 + 0].position = mul(v0.position, mvp);
    vertices[i0 + 1].position = mul(v1.position, mvp);
    vertices[i0 + 2].position = mul(v2.position, mvp);

    vertices[i0 + 0].uv = v0.uv;
    vertices[i0 + 1].uv = v1.uv;
    vertices[i0 + 2].uv = v2.uv;

    triangles[tid.x] = uint3(i0, i0+1, i0+2);
}

// ╭────────────────────────────────────────────╮
// │               Fragment Shader              │
// ╰────────────────────────────────────────────╯
[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_TARGET
{
    float4 color = sampledImage[NonUniformResourceIndex(textureId)].Sample(samp, input.uv);
    return color;
}
