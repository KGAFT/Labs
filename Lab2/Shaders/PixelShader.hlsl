static const float ambientIntensity = 0.1f;
static const float diffuseIntensity = 0.5f;
static const float constant = 0.0f;
static const float linearData = 0.1f;
static const float exponential = 0;
static const float specularIntensity = 1.0f;
static const float specularPower = 32;

struct PixelShaderInput
{
    float4 position: SV_POSITION;
    float3 normal: NORMAL;
    float2 uv: UV;
    float4 color: COLOR;
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

float4 loadBaseLight(PointLightSource light, float3 fragmentPosition, float3 direction, float3 normals){
    float curAmbientIntesity = light.intensity;
    float curDiffuseIntesity = light.intensity*diffuseIntensity;
    float curSpecularIntensity = light.intensity*specularIntensity;
    float4 ambientColor = float4(1,1,1,1) *
    curAmbientIntesity;
    float diffuseFactor = dot(normalize(normals), -direction);

    float4 diffuseColor;
    float4 specularColor = float4(0, 0, 0, 0);
    if (diffuseFactor > 0) {
        float3 vertexToEye = normalize(cameraPosition-fragmentPosition);
        diffuseColor = float4(1,1,1,1) *
        curDiffuseIntesity *
        diffuseFactor;
        diffuseColor.w = 1;
        float3 lightReflect = normalize(reflect(direction, normals));
        float specularFactor = dot(vertexToEye, lightReflect);
        if(specularFactor>0){
            specularFactor = pow(specularFactor, specularPower);
            specularColor = float4(1,1,1,1) * curSpecularIntensity * specularFactor;
            specularColor.w = 1;
        }
    }
    else {
        diffuseColor = float4(0, 0, 0, 0);
    }
    return (ambientColor+diffuseColor+specularColor);
}

float4 calcPointLight(PointLightSource pointLight, float3 fragPos, float3 normals){
    float3 lightDirection = cameraPosition-pointLight.position;
    float lightDistance = length(lightDirection);
    lightDirection = normalize(lightDirection);
    float4 baseColor = loadBaseLight(pointLight, fragPos, lightDirection, normals);
    float attenuationFactor = constant+linearData*lightDistance+exponential*lightDistance*lightDistance;
    return baseColor/attenuationFactor;
}


float4 main(PixelShaderInput psInput) : SV_Target
{
    float3 normal = normalize(psInput.normal);

    float4 lightImpact = float4(0, 0, 0, 0);
    for (uint i = 0; i < 3; i++)
    {
        lightImpact += calcPointLight(lightsInfos[i], psInput.position, normal);
    }

    float3 finalColor = psInput.color * lightImpact;
    return float4(finalColor, 1.0);
}
