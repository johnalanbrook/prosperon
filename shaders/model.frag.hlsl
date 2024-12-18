// If using a texture instead, define:
Texture2D diffuse : register(t0, space2);
SamplerState smp : register(s0, space2);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
  return float4(input.uv,1,1);
}
