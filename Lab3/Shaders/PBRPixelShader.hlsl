struct PixelShaderInput
{
    float4 position: SV_POSITION;
    float3 normal: NORMAL;
    float2 uv: UV;
    float3 color: COLOR;
};

struct PointLight
{
    float3 position;
    float intensity;
};

cbuffer LightData: register(b0)
{
    PointLight lightsInfos[3];
    float3 cameraPosition;
};

cbuffer Configuration: register(b1)
{
    int defaultFunction;
    int normalDistribution;
    int fresnelFunction;
    int geometryFunction;

    float metallic;
    float roughness;
    float ambientIntensity;
    float alignment;
};

TextureCube cubeMap : register (t0);
SamplerState cubeMapSampler : register (s0);

static const float PI = 3.14159265359;


///

float3 fresnelFunctionMetal(float3 objColor, float3 h, float3 v, float metalness)
{
    float3 f = float3(0.04f, 0.04f, 0.04f) * (1 - metalness) + objColor * metalness;
    return f + (float3(1.0f, 1.0f, 1.0f) - f) * pow(1.0f - max(dot(h, v), 0.0f), 5);
}

float distributeGGX(float3 n, float3 h, float roughness)
{
    float num = roughness * roughness;
    float denom = max(dot(n, h), 0.0f);
    denom = denom * denom * (num - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return num / denom;
}

float schlickGeometryGGX(float nvl, float roughness)
{
    float k = roughness + 1.0f;
    k = k * k / 8.0f;

    return nvl / (nvl * (1.0f - k) + k);
}

float smithGeometry(float3 n, float3 v, float3 l, float roughness)
{
    return schlickGeometryGGX(max(dot(n, v), 0.0001f), roughness) * schlickGeometryGGX(
        max(dot(n, l), 0.0001f), roughness);
}

///


float3 processPointLight(PointLight light, float3 normals, float3 fragmentPosition, float3 worldViewVector,
                         float3 startFresnelSchlick, float roughness, float metallic, float3 albedo)
{
    float3 lightDir = light.position.xyz - fragmentPosition;
    float lightDist = length(lightDir);
    lightDir /= lightDist;
    float atten = clamp(1.0 / (lightDist * lightDist), 0, 1.0f);
    float3 radiance = float3(1, 1, 1) * light.intensity * atten;

    float halfWayGGX = distributeGGX(normals, normalize((worldViewVector + lightDir) / 2.0f), roughness);;
    float geometrySmith = smithGeometry(normals, worldViewVector, lightDir, roughness);


    float3 fresnelSchlick = fresnelFunctionMetal(albedo, normalize((worldViewVector + lightDir) / 2.0f),
                                                 worldViewVector, metallic);

    float3 numerator = float3(0, 0, 0);
    float denominator = 1;
    if (defaultFunction)
    {
        numerator = halfWayGGX * geometrySmith * fresnelSchlick;
        denominator = 4.0 * max(dot(normals, worldViewVector), 0.0) * radiance * max(dot(normals, lightDir), 0.0) +
            0.0001;
    }
    else if (fresnelFunction)
    {
        numerator = fresnelSchlick;
        denominator = 4.0 * max(dot(normals, worldViewVector), 0.0) * max(dot(normals, lightDir), 0.0) +
            0.0001;;
    }
    else if (normalDistribution)
    {
        numerator = float3(halfWayGGX, halfWayGGX, halfWayGGX);
    }
    else if (geometryFunction)
    {
        numerator = float3(geometrySmith, geometrySmith, geometrySmith);
    }
    float3 kd = float3(1.0f, 1.0f, 1.0f) - fresnelSchlick;
    kd = kd * (1.0f - metallic);
    float3 specular = numerator / denominator;
    if (fresnelFunction || defaultFunction)
    {
        return kd * albedo / PI + specular;
    }

    return specular;
}


float4 main(PixelShaderInput psInput) : SV_Target
{
    float3 finalColor;
    if (defaultFunction)
    {
        finalColor = psInput.color;
    }
    else
    {
        finalColor = float3(0.0f, 0.0f, 0.0f);
    }
    float3 normal = normalize(psInput.normal);

    float3 viewDir = normalize(cameraPosition - psInput.position);
    float pi = 3.14159265359f;
    for (int i = 0; i < 3; i++)
    {
        float3 lightDir = lightsInfos[i].position - psInput.position;
        float lightDist = length(lightDir);
        lightDir /= lightDist;
        float atten = clamp(1.0 / (lightDist * lightDist), 0, 1.0f);
        float3 radiance = float3(1, 1, 1) * lightsInfos[i].intensity * atten;


        float3 F = fresnelFunctionMetal(psInput.color, normalize((viewDir + lightDir) / 2.0f), viewDir, metallic);

        float NDF = distributeGGX(normal, normalize((viewDir + lightDir) / 2.0f), roughness);


        float G = smithGeometry(normal, viewDir, lightDir, roughness);


        float3 kd = float3(1.0f, 1.0f, 1.0f) - F;
        kd = kd * (1.0f - metallic);

        float3 lightImpact;
        if (defaultFunction)
        {
            lightImpact = (kd * normal / pi +
                    NDF * G * F / max(4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f),
                                      0.0001f)) *
                radiance * max(dot(normal, lightDir), 0.0f);
        }
        else if (fresnelFunction)
        {
            lightImpact = (F / max(4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDir), 0.0f),
                                   0.0001f)) *
                max(dot(normal, lightDir), 0.0f);
        }
        else if (normalDistribution)
        {
            lightImpact = float3(NDF, NDF, NDF);
        }
        else if (geometryFunction)
        {
            lightImpact = float3(G, G, G);
        }

        finalColor += lightImpact * lightsInfos[i].intensity;
    }
    return float4(finalColor, 1.0f);
}
