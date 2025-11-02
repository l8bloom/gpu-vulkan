struct VSInput {
  float3 inPosition;
  float3 inColor;
  float2 inTexCoord;
};

struct UniformBuffer{
  float4x4 model;
  float4x4 view;
  float4x4 proj;
};

ConstantBuffer<UniformBuffer> ubo;

struct VSOutput {
  float4 pos: SV_Position;
  float3 color;
  float2 fragTexCoord;
};


[shader("vertex")]
VSOutput vertMain(VSInput input) {
  VSOutput output;
  output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0))));
  output.color = input.inColor;
  output.fragTexCoord = input.inTexCoord;
  return output;
}

[[vk::binding(1, 0)]]
Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target{
  //return float4(0.2 * color.b, 0.5* color.g, 0.3 * color.r, 1);
  //return float4(0.0, 1.0, 0.0, 1.0);
  return float4(vertIn.color * texture.Sample(vertIn.fragTexCoord).rgb, 1.0);
  return texture.Sample(vertIn.fragTexCoord);
  return float4(vertIn.color, 1.0);
}
