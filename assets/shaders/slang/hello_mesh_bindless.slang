import modules.bindless;
import gen.hello_mesh_bindless;

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



VertexIn GetVertex(uint indexId)
{
    let vertexId = handle::index_cube.get<uint>(indexId);
    let vertex = handle::vertex_cube.get<VertexIn>(vertexId);
    return vertex;
}

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
    uint primId = tid.x;

    let mat = handle::transform_cube.get<TransformMatrix>();

    int i0 = primId * 3;

    VertexIn v0 = GetVertex(i0);
    VertexIn v1 = GetVertex(i0 + 1);
    VertexIn v2 = GetVertex(i0 + 2);

    vertices[i0 + 0].position = mat.transform(v0.position);
    vertices[i0 + 1].position = mat.transform(v1.position);
    vertices[i0 + 2].position = mat.transform(v2.position);

    vertices[i0 + 0].uv = v0.uv;
    vertices[i0 + 1].uv = v1.uv;
    vertices[i0 + 2].uv = v2.uv;

    triangles[primId] = uint3(i0, i0+1, i0+2);
}

// ╭────────────────────────────────────────────╮
// │               Fragment Shader              │
// ╰────────────────────────────────────────────╯
[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_TARGET
{
    // equivalent:
    /* let samp = bindless::getSampler(handleData.samp); */
    /* let tex = bindless::getSampledImage(handleData.texture); */
    /* float4 color = tex.Sample(samp, input.uv); */
    let color = handle::texture_container.sample(handle::samp, input.uv);

    return color;
}
