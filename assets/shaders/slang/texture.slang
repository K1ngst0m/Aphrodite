// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex
{
    float2 uv : TEXCOORD0;
};

// Output of the fragment shader
struct Fragment
{
    float4 color : SV_TARGET;
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

    float3 position = assembledVertex.position;
    float2 uv       = assembledVertex.uv;

    output.coarseVertex.uv = uv;
    output.sv_position     = float4(position, 1.0);

    return output;
}

[[vk::binding(0, 0)]] Texture2D tex;
[[vk::binding(1, 0)]] SamplerState samp;

// Fragment Shader
[shader("fragment")]
float4 fragmentMain(CoarseVertex coarseVertex : CoarseVertex) : SV_Target
{
    float4 color = tex.Sample(samp, coarseVertex.uv);

    return color;
}
