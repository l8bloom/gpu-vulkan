static float2 positions[6] = float2[](
  float2(-0.25, -0.5),
  float2(0.25, 0.5),
  float2(-0.25, 0.5),

  float2(-0.25, -0.5),
  float2(0.25, -0.5),
  float2(0.25, 0.5)
);

static float3 colors[3] = float3[](
  float3(1.0, 0.0, 0.0),
  float3(0.0, 1.0, 0.0),
  float3(0.0, 0.0, 1.0)
);

struct VertexOutput {
  float4 sv_position: SV_Position;
  float3 color;
};


[shader("vertex")]
VertexOutput vertMain(uint vid: SV_VertexID) {
  VertexOutput output;
  output.sv_position = float4(positions[vid], 0.0, 1.0);
  output.color = colors[vid % 3];
  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target{
  float3 color = inVert.color;
  //return float4(0.2 * color.b, 0.5* color.g, 0.3 * color.r, 1);
  return float4(0.0, 1.0, 0.0, 1.0);
  //return float4(color, 1.0);
}
