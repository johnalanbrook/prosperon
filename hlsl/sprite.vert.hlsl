// Constant Buffer for Transformation Matrices
cbuffer TransformBuffer : register(b0)
{
    float4x4 WorldMatrix;        // World transformation matrix
    float4x4 ViewProjectionMatrix; // Combined view and projection matrix
}

// Input structure for the vertex shader
struct VertexInput
{
    float3 Position : POSITION; // Vertex position (model space)
    float2 UV       : TEXCOORD0; // Texture coordinates
    float4 Color    : COLOR0;    // Vertex color
};

// Output structure from the vertex shader to the pixel shader
struct VertexOutput
{
    float4 Position : SV_Position; // Clip-space position
    float2 UV       : TEXCOORD0;   // Texture coordinates
    float4 Color    : COLOR0;      // Interpolated vertex color
};

// Vertex shader
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Transform the vertex position from model space to world space
    float4 worldPosition = mul(WorldMatrix, float4(input.Position, 1.0f));

    // Transform the position to clip space using the view-projection matrix
    output.Position = mul(ViewProjectionMatrix, worldPosition);

    // Pass through texture coordinates and vertex color
    output.UV = input.UV;
    output.Color = input.Color;

    return output;
}
