// Per-vertex attributes to be assembled from bound vertex buffers.

struct DummyPayLoad
{
    uint dummyData;
};

// We don't use pay loads in this sample, but the fn call requires one
groupshared DummyPayLoad dummyPayLoad;

[numthreads(1,1,1)]
[shader("task")]
void taskMain()
{
    DispatchMesh(3, 1, 1, dummyPayLoad);
}

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VertexOutput
{
    float4 position: SV_Position;
    float4 color: COLOR0;
};

static const float4 positions[3] = {
    float4( 0.0, -1.0, 0.0, 1.0),
    float4(-1.0,  1.0, 0.0, 1.0),
    float4( 1.0,  1.0, 0.0, 1.0)
};

static const float4 colors[3] = {
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
    float4(1.0, 0.0, 0.0, 1.0)
};

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void main(out indices uint3 triangles[1], out vertices VertexOutput vertices[3], uint3 DispatchThreadID : SV_DispatchThreadID)
{
    float4x4 mvp = mul(ubo.projection, mul(ubo.view, ubo.model));

    float4 offset = float4(0.0, 0.0, (float)DispatchThreadID, 0.0);

    SetMeshOutputCounts(3, 1);
    for (uint i = 0; i < 3; i++) {
        vertices[i].position = mul(mvp, positions[i] + offset);
        vertices[i].color = colors[i];
    }

    SetMeshOutputCounts(3, 1);
    triangles[0] = uint3(0, 1, 2);
}

struct VSOutput
{
[[vk::location(0)]] float4 color : COLOR0;
};

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET
{
    return float4(input.color);
}
