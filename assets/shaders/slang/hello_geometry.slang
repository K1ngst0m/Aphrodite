// ╭────────────────────────────────────────────╮
// │               Vertex Shader                │
// ╰────────────────────────────────────────────╯
// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex
{
    float4 position : POSITION;
    float4 uv : TEXCOORD0;
};

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex
{
    float2 uv : TEXCOORD0;
};

[[vk::binding(0, 0)]] cbuffer ubo
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

// Vertex  Shader
struct VertexStageOutput
{
    CoarseVertex coarseVertex : CoarseVertex;
    float4       sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float4 position = assembledVertex.position;
    float2 uv       = assembledVertex.uv.xy;

    float4x4 mvp = mul(mul(model, view), projection);

    output.coarseVertex.uv = uv;
    output.sv_position     = mul(position, mvp);

    return output;
}

// ╭────────────────────────────────────────────╮
// │               Fragment Shader              │
// ╰────────────────────────────────────────────╯
[[vk::binding(0, 1)]] Texture2D tex;
[[vk::binding(1, 1)]] SamplerState samp;

[shader("fragment")]
float4 fragMain(CoarseVertex coarseVertex : CoarseVertex) : SV_Target
{
    float4 color = tex.Sample(samp, coarseVertex.uv);
    return color;
}
