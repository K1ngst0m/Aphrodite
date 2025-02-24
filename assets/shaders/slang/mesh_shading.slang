struct VertexOutput
{
    float4 position : SV_Position;
    float4 color: COLOR0;
};

static const float4 colors[4] = {
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
    float4(1.0, 0.0, 0.0, 1.0),
    float4(1.0, 1.0, 0.0, 1.0)
};

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
[shader("mesh")]
void meshMain(out indices uint3 triangles[2], out vertices VertexOutput vertices[4], uint3 DispatchThreadID : SV_DispatchThreadID)
{
    SetMeshOutputCounts(4, 2);
    vertices[0].position = float4(0.5, -0.5, 0, 1);
    vertices[1].position = float4(0.5, 0.5, 0, 1);
    vertices[2].position = float4(-0.5, 0.5, 0, 1);
    vertices[3].position = float4(-0.5, -0.5, 0, 1);
    vertices[0].color = colors[0];
    vertices[1].color = colors[1];
    vertices[2].color = colors[2];
    vertices[3].color = colors[3];
    triangles[0] = uint3(0, 1, 2);
    triangles[1] = uint3(0, 2, 3);
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_TARGET
{
  return float4(input.color);
}
