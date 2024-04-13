struct PixelShaderInput
{
    float4 position: SV_POSITION;
    float3 normal: NORMAL;
    float2 uv: UV;
    float4 color: COLOR;
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


float distributeGGX(float3 normals, float3 halfWayVector, float roughness)
{
    float roughness4 = roughness * roughness * roughness * roughness;
    float halwayDot2 = max(dot(normals, halfWayVector), 0.0) * max(dot(normals, halfWayVector), 0.0);
    float numerator = roughness4;
    float denominator = (halwayDot2 * (roughness4 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    return numerator / denominator;
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

float3 fresnelSchlickRoughness(float cosTheta, float3 startFresnelSchlick, float roughness)
{
    return startFresnelSchlick + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), startFresnelSchlick) -
        startFresnelSchlick) * pow(1.0 - cosTheta, 5.0);
}

float3 processPointLight(PointLight light, float3 normals, float3 fragmentPosition, float3 worldViewVector,
                         float3 startFresnelSchlick, float roughness, float metallic, float3 albedo)
{
    float3 processedLightPos = normalize(light.position - fragmentPosition);
    float3 halfWay = normalize(worldViewVector + processedLightPos);
    float distance = length(light.position - fragmentPosition);
    float attenuation = clamp(1.0 / (distance * distance), 0, 1.0f);
    float3 radiance = float3(1, 1, 1) * light.intensity * attenuation;

    float halfWayGGX = distributeGGX(normals, halfWay, roughness);
    float geometrySmith = smithGeometry(normals, worldViewVector, processedLightPos, roughness);
    float3 fresnelSchlick = fresnelSchlickBase(clamp(dot(halfWay, worldViewVector), 0.0, 1.0), startFresnelSchlick);

    float3 numerator = float3(0, 0, 0);
    float denominator = 1;
    if (defaultFunction)
    {
        numerator = halfWayGGX * geometrySmith * fresnelSchlick;
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
        finalFresnelSchlick *= 1.0 - metallic;

        float NdotL = max(dot(normals, processedLightPos), 0.0);
        return (finalFresnelSchlick * albedo / PI + specular) * radiance * NdotL;
    }
    else
    {
        return specular;
    }
}

float3 getReflection(float roughness, float3 reflectanceVec)
{
    const float MAX_REFLECTION_LOD = 9.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    float3 a = cubeMap.SampleLevel(cubeMapSampler, reflectanceVec, lodf).rgb;
    float3 b = cubeMap.SampleLevel(cubeMapSampler, reflectanceVec, lodc).rgb;
    return lerp(a, b, lod - lodf);
}


float4 main(PixelShaderInput psInput) : SV_Target
{
    float3 normal = psInput.normal;
    //Extract to constant buffer

    //

    float3 worldViewVector = normalize(cameraPosition - psInput.position);
    float3 startFresnelSchlick = float3(0.04, 0.04, 0.04);
    startFresnelSchlick = lerp(startFresnelSchlick, psInput.color, metallic);
    float3 Lo = float3(0, 0, 0);
    float3 reflection = getReflection(roughness, reflect(worldViewVector, normal));
    for (uint i = 0; i < 3; i++)
    {
        Lo += processPointLight(lightsInfos[i], normal, psInput.position, worldViewVector, startFresnelSchlick,
                                roughness, metallic, psInput.color);
    }
    float3 fresnelRoughness = fresnelSchlickRoughness(max(dot(normal, worldViewVector), 0.0), startFresnelSchlick,
                                                      roughness);
    float3 specularContribution = reflection * fresnelRoughness * 0.025f;

    float3 ambient = (float3(ambientIntensity, ambientIntensity, ambientIntensity) * psInput.color + fresnelRoughness +
        specularContribution);
    float3 color = ambient + Lo;
    return float4(color, 1.0f);
}
