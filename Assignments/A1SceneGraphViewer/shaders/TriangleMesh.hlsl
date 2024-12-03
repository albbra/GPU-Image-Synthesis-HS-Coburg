// TriangleMesh.hlsl

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 clipSpacePosition : SV_POSITION;
    float3 viewSpacePosition : POSITION;
    float3 viewSpaceNormal : NORMAL;
    float2 texCoord : TEXCOOD;
};

struct Light
{
    float3 position;
    float  pad1;
    float3 color;
    float  intensity;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
    float3   cameraPosition;
    int      numOfLights;
    Light    lights[8];
}

/// <summary>
/// Constants that can change per Mesh/Drawcall.
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
}

/// <summary>
/// Constants that are really constant for the entire scene.
/// </summary>
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

    float4 p4 = mul(modelViewMatrix, float4(input.position, 1.0f));
    output.viewSpacePosition = p4.xyz;
    output.viewSpaceNormal = mul(modelViewMatrix, float4(input.normal, 0.0f)).xyz;
    output.clipSpacePosition = mul(projectionMatrix, p4);
    output.texCoord = input.texCoord;

    return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
    float3 sampledAmbientColor  = g_textureAmbient.Sample(g_sampler, input.texCoord, 0);
    float3 sampledDiffuseColor  = g_textureDiffuse.Sample(g_sampler, input.texCoord, 0);
    float3 sampledSpecularColor = g_textureSpecular.Sample(g_sampler, input.texCoord, 0);
    float3 sampledEmissiveColor = g_textureEmissive.Sample(g_sampler, input.texCoord, 0);
    float3 sampledNormals       = g_textureNormal.Sample(g_sampler, input.texCoord, 0);
    
    float3 mixedAmbientColor  = sampledAmbientColor * ambientColor.rgb;
    float3 mixedDiffuseColor  = sampledDiffuseColor * diffuseColor.rgb;
    float3 mixedSpecularColor = sampledSpecularColor + specularColorAndExponent.rgb;
    float3 mixedEmissiveColor = sampledEmissiveColor * emissiveColor.rgb;
    float  exponent           = specularColorAndExponent.a;
    
    float3 normal = normalize(input.viewSpaceNormal);
    float3 viewDirection = normalize(cameraPosition - input.viewSpacePosition);

    // Initialize lighting components
    float3 ambientLighting = mixedAmbientColor;
    float3 diffuseLighting = float3(0.0f, 0.0f, 0.0f);
    float3 specularLighting = float3(0.0f, 0.0f, 0.0f);

    // Accumulate lighting from all lights
    for (int i = 0; i < numOfLights; ++i)
    {
        float3 lightDirection = normalize(lights[i].position - input.viewSpacePosition);
        float diffuseFactor = max(dot(normal, lightDirection), 0.0f);

        // Diffuse lighting
        diffuseLighting += (mixedDiffuseColor * lights[i].color * diffuseFactor) * lights[i].intensity;

        // Specular lighting
        float3 reflectionVector = reflect(-lightDirection, normal);
        float specularFactor = max(pow(max(dot(reflectionVector, viewDirection), 0.0f), exponent), 0.0f);
        specularLighting += (mixedSpecularColor * lights[i].color * specularFactor) * lights[i].intensity;
    }

    // Combine all lighting components
    float3 finalColor = ambientLighting + diffuseLighting + specularLighting + mixedEmissiveColor;

    return float4(finalColor.rgb, 1.0f);
}
