#include "Shaders/BaseHDR.hlsli"

struct PixelShaderInput
{
    float4 position: SV_POSITION;
    float3 normal: NORMAL;
    float2 uv: UV;
};

struct PointLightSource
{
    float3 position;
    float intensity;
};

cbuffer LightData: register(b0)
{
    PointLightSource lightsInfos[3];
    float3 cameraPosition;
};

float3 calculateLight(PointLightSource source, float3 fragPos, float3 normal, float3 viewDirection, float3 color)
{
    float3 lightDirection = normalize(source.position - fragPos);
    float diff = max(dot(lightDirection, normal), 0.0);
    float3 diffuse = diff * color;

    float3 reflectionDirection = reflect(-lightDirection, normal);
    float specular = 0.0;
    float3 halfwayDirection = normalize(lightDirection + viewDirection);

    specular = pow(max(dot(normal, halfwayDirection), 0.0), 8.0);
    float3 finalSpecular = float3(0.3, 0.3, 0.3) * specular;
    return (diffuse + finalSpecular) * source.intensity;
}

float4 main(PixelShaderInput psInput) : SV_Target
{
    float3 normal = normalize(psInput.normal);
    float3 ambient = 0.05 * psInput.position;
    float3 viewDirection = normalize(cameraPosition - psInput.position);

    float3 lightImpact = float3(0, 0, 0);
    for (uint i = 0; i < 3; i++)
    {
        lightImpact += calculateLight(lightsInfos[i], psInput.position, normal, viewDirection, psInput.normal);
    }

    float3 finalColor = ambient + lightImpact;
    return float4(finalColor, 1.0);
}
