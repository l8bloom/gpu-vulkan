struct VSInput {
  float2 inPosition;
  float3 inColor;
};

struct VSOutput {
  float4 pos: SV_Position;
  float3 color;
};


[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  output.pos = float4(input.inPosition, 0.0, 1.0);
  output.color = input.inColor;
  return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target{
  //return float4(0.2 * color.b, 0.5* color.g, 0.3 * color.r, 1);
  return float4(0.0, 1.0, 0.0, 1.0);
  return float4(vertIn.color, 1.0);
}
