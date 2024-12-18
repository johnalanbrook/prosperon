Texture2D diffuse : register(t0, space2);
SamplerState smp : register(s0, space2);

// Structure for pixel shader input (from vertex shader output).
struct PSInput
{
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

// Pixel shader main function
float4 main(PSInput input) : SV_TARGET
{
    float4 color = diffuse.Sample(smp, input.uv);
    color *= input.color;

    if (color.a == 0.0f)
        clip(-1);

    return color;
}