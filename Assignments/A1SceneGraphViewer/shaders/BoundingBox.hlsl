// BoundingBox.hlsl
struct VertexInput
{
    float3 position : POSITION;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
};

struct Light
{
    float3 position;
    float pad1;
    float3 color;
    float intensity;
};

cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
    float3 cameraPosition;
    int numOfLights;
    Light lights[8];
}

cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
}

cbuffer Material : register(b2)
{
    float4 ambientColor;
    float4 diffuseColor;
    float4 emissiveColor;
    float4 specularColorAndExponent;
}

Texture2D<float4> g_textureAmbient : register(t0);
Texture2D<float4> g_textureDiffuse : register(t1);
Texture2D<float4> g_textureSpecular : register(t2);
Texture2D<float4> g_textureEmissive : register(t3);
Texture2D<float4> g_textureNormal : register(t4);

SamplerState g_sampler : register(s0);

VertexShaderOutput VS_main(VertexInput input)
{
    VertexShaderOutput output;

    // Apply the model-view transformation followed by the projection transformation
    float4 worldPosition = mul(float4(input.position, 1.0f), modelViewMatrix); // Model to Camera space
    output.position = mul(worldPosition, projectionMatrix); // Camera space to Clip space
    
    return output;
}

float4 PS_main() : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f); // White color for bounding box
}
