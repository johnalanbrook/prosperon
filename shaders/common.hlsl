// Constant Buffer for Transformation Matrices
cbuffer TransformBuffer : register(b0, space1)
{
    float4x4 vp;
    float4x4 view;
    float time; // time in seconds since app start
}

struct VSInput
{
  float3 pos : POSITION;
  float2 uv : TEXCOORD0;
  float4 color : COLOR0;
  float3 normal : NORMAL;
};