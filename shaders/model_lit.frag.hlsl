cbuffer LightBuffer : register(b2, space1)
{
    float4 uDiffuseColor; // base diffuse color
    float3 uLightDirection; // direction of the incoming light, normalized
    float4 uLightColor;     // color of the light, e.g., (1,1,1,1)
};

// Example hard-coded values (if not using cbuffer, you can directly hardcode in code):
// float3 uLightDirection = normalize(float3(0.0, -1.0, -1.0));
// float4 uLightColor = float4(1.0, 1.0, 1.0, 1.0);

// You can also still use a texture:
Texture2D uDiffuseTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
    // Sample base color
    float4 baseColor = uDiffuseTexture.Sample(samplerState, input.uv);
    // If no texture, just use uDiffuseColor:
    // float4 baseColor = uDiffuseColor;

    // Compute diffuse lighting
    // Ensure normal is normalized
    float3 N = normalize(input.normal);
    // Light direction is assumed normalized. Make sure uLightDirection is a direction from fragment to light.
    // If the direction is from the light to the fragment, invert it: float3 L = -uLightDirection;
    float NdotL = saturate(dot(N, -uLightDirection));

    float4 shadedColor = baseColor * (uLightColor * NdotL);

    return shadedColor;
}
