#include "common.hlsl"

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;  // Might be unused in no-light case, but we'll pass it anyway.
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(vp, float4(input.pos, 1.0));
    output.uv = input.uv;
    output.normal = input.normal; // World space normal if needed
    return output;
}
