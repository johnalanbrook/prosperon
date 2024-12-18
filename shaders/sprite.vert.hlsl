#include "common.hlsl"

// Output structure from the vertex shader to the pixel shader
struct VertexOutput
{
    float4 pos : SV_Position; // Clip-space position
    float2 uv       : TEXCOORD0;   // Texture coordinates
    float4 color    : COLOR0;      // Interpolated vertex color
};

// Vertex shader
VertexOutput main(VSInput input)
{
    VertexOutput output;
    output.pos = mul(vp, float4(input.pos,1.0));
    output.uv = input.uv;
    output.color = input.color;

    return output;
}

