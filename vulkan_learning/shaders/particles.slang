struct VSInput{
  float2 inPosition;
  float4 inColor;
};

struct VSOutput{
  float4 position: SV_Position;
  float pointSize : SV_PointSize;
  float3 fragmentColor : COLOR0;
};

[shader("vertex")]
VSOutput vertexMain(VSInput vertexInput){
  VSOutput output;
  output.position = float4(vertexInput.inPosition, 1.0, 1.0);
  output.pointSize = 14.0;
  output.fragmentColor = vertexInput.inColor.rgb;
  return output;
}

struct PSInput{
  float4 pos : SV_POSITION;
  float3 fragmentColor : COLOR0;
  float2 pointCoord : SV_PointCoord;
};

[shader("fragment")]
float4 fragmentMain(PSInput fragInput){
  float2 coord = fragInput.pointCoord - float2(0.5);
  return float4(fragInput.fragmentColor, 0.5 - length(coord));
}

struct UniformBuffer{
  float deltaTime;
};

ConstantBuffer<UniformBuffer> ubo;

struct Particle{
  float2 position;
  float2 velocity;
  float4 color;
};

struct ParticleSSBO{
  Particle particles;
};

StructuredBuffer<ParticleSSBO> particlesIn;
RWStructuredBuffer<ParticleSSBO> particlesOut;


[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 threadId : SV_DispatchThreadID){
  uint index = threadId.x;
  particlesOut[index].particles.position = particlesIn[index].particles.position + particlesIn[index].particles.velocity.xy * ubo.deltaTime;
  particlesOut[index].particles.velocity = particlesIn[index].particles.velocity;

  // Flip movement at window border
  if ((particlesOut[index].particles.position.x <= -1.0) || (particlesOut[index].particles.position.x >= 1.0)) {
      particlesOut[index].particles.velocity.x = -particlesOut[index].particles.velocity.x;
  }
  if ((particlesOut[index].particles.position.y <= -1.0) || (particlesOut[index].particles.position.y >= 1.0)) {
      particlesOut[index].particles.velocity.y = -particlesOut[index].particles.velocity.y;
  }
}

