TextureCube irradianceTexture : register (t0);
SamplerState irradianceSampler : register (s0);

struct PixelShaderInput
{
    float4 position: SV_POSITION;
    float4 worldPos: WORLDPOS;
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


static const float PI = 3.14159265359f;


float distributeGGX(float3 normals, float3 halfWayVector, float roughness)
{
    float a = roughness * roughness;
    float NdotH = max(dot(normals, halfWayVector), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a;
    float denom = (NdotH2 * (nom - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float schlickGeometryGGX(float dotWorldViewVector, float roughness)
{
    float roughnessKoef = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    float numerator = dotWorldViewVector;
    float denominator = dotWorldViewVector * (1.0 - roughnessKoef) + roughnessKoef;
    return numerator / denominator;
}

float smithGeometry(float3 processedNormals, float3 worldViewVector, float3 lightPosition, float roughness)
{
    float worldViewVectorDot = max(dot(processedNormals, worldViewVector), 0.0);
    float lightDot = max(dot(processedNormals, lightPosition), 0.0);
    float ggx2 = schlickGeometryGGX(worldViewVectorDot, roughness);
    float ggx1 = schlickGeometryGGX(lightDot, roughness);
    return ggx1 * ggx2;
}

float3 fresnelSchlickBase(float cosTheta, float3 startFresnelSchlick)
{
    return startFresnelSchlick + (1.0 - startFresnelSchlick) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 fresnelFunctionMetal(float3 objColor, float3 startFresnelSchlick,  float3 h, in float3 worldViewVector, float metalllic)
{
    float3 f = startFresnelSchlick * (1 - metalllic) + objColor * metalllic;
    return f + (float3(1.0f, 1.0f, 1.0f) - f) * pow(1.0f - max(dot(h, worldViewVector), 0.0f), 5);
}

float3 processPointLight(PointLight light, float3 normals, float3 fragmentPosition, float3 worldViewVector,
                         float3 startFresnelSchlick, float roughness, float metallic, float3 albedo)
{
    float3 processedLightPos = normalize(light.position - fragmentPosition);
    float distance = length(light.position - fragmentPosition);
    float3 halfWay = normalize((worldViewVector + processedLightPos)/2.0f);
    
    float attenuation = clamp(1.0 / (distance * distance), 0.01, 1.0f);
    float3 radiance = float3(1, 1, 1)* light.intensity *attenuation;

    float halfWayGGX = distributeGGX(normals, halfWay, roughness);
    float geometrySmith = smithGeometry(normals, worldViewVector, processedLightPos, roughness);
    
    
    float3 fresnelSchlick = fresnelFunctionMetal(albedo, startFresnelSchlick, halfWay, worldViewVector, metallic);

    float3 numerator = float3(0, 0, 0);
    float denominator = 1;
    if (defaultFunction)
    {
        numerator = halfWayGGX*geometrySmith * fresnelSchlick;
        denominator = 4.0 * max(dot(normals, worldViewVector), 0.0) * max(dot(normals, processedLightPos), 0.0) +
            0.0001;
    }
    else if (fresnelFunction)
    {
        numerator = fresnelSchlick;
        denominator = 4.0 * max(dot(normals, worldViewVector), 0.0) * max(dot(normals, processedLightPos), 0.0) +
            0.0001;;
    }
    else if (normalDistribution)
    {
        numerator = float3(halfWayGGX, halfWayGGX, halfWayGGX);
        denominator = 1.0f;
    }
    else if (geometryFunction)
    {
        numerator = float3(geometrySmith, geometrySmith, geometrySmith);
        denominator = 1.0f;
    }


    float3 specular = numerator / denominator;
    if (defaultFunction)
    {
       
        float3 finalFresnelSchlick = float3(1, 1, 1) - fresnelSchlick;
        finalFresnelSchlick *= 1.0 - metallic+0.001;

        float NdotL = max(dot(normals, processedLightPos), 0.0);
        return (finalFresnelSchlick * albedo / PI + specular) * radiance * NdotL;
    }
    else
    {
        return specular;
    }
}


float4 main(PixelShaderInput psInput) : SV_Target
{
    float3 normal = normalize(psInput.normal);
    float3 worldViewVector = normalize(cameraPosition - psInput.worldPos);
    float3 irradiance = irradianceTexture.Sample(irradianceSampler, normal).xyz;
    
    float3 startFresnelSchlick = float3(0.04, 0.04, 0.04);
    startFresnelSchlick = lerp(startFresnelSchlick, irradiance, metallic);
    float3 Lo = float3(0, 0, 0);
    for (uint i = 0; i < 3; i++)
    {
        Lo += processPointLight(lightsInfos[i], normal, psInput.worldPos, worldViewVector, startFresnelSchlick,
                                roughness, metallic, irradiance);
    }
   

    float3 ambient = (float3(ambientIntensity, ambientIntensity, ambientIntensity) * irradiance);
    float3 rescolor = ambient + Lo;
    
    return float4(rescolor, 1.0f);
}