TextureCube cubeTexture : register (t0);
SamplerState cubeSampler : register (s0);

cbuffer roughnessBuffer : register (b0)
{
    float4 roughness;
}

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 localPos : POSITION;
};

static float PI = 3.14159265359f;

float random(float2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, float2(a, b));
    float sn = fmod(dt, 3.14);
    return frac(sin(sn) * c);
}

float2 hammersley2d(uint i, uint N)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return float2(float(i) / float(N), rdi);
}

float3 importanceSampleGGX(float2 Xi, float roughness, float3 normal)
{
    float alpha = roughness * roughness;
    float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangentX = normalize(cross(up, normal));
    float3 tangentY = normalize(cross(normal, tangentX));

    return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

float distributeGGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (PI * denom * denom);
}

float3 prefilterEnvMap(float3 R, float roughness)
{
    float3 N = R;
    float3 V = R;
    float3 color = float3(0, 0, 0);
    float totalWeight = 0.0;
    float width = 128;
    float envMapDim = width;
    static const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = hammersley2d(i, SAMPLE_COUNT);
        float3 H = importanceSampleGGX(Xi, roughness, N);
        float3 L = 2.0 * dot(V, H) * H - V;
        float dotNL = clamp(dot(N, L), 0.0, 1.0);
        if (dotNL > 0.0)
        {
            float dotNH = clamp(dot(N, H), 0.0, 1.0);
            float dotVH = clamp(dot(V, H), 0.0, 1.0);

            float pdf = distributeGGX(dotNH, roughness) * dotNH / (4.0 * dotVH) + 0.0001;
            float omegaS = 1.0 / (float(SAMPLE_COUNT) * pdf);
            float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);
            float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f);
            color += cubeTexture.SampleLevel(cubeSampler, L, mipLevel).rgb * dotNL;
            totalWeight += dotNL;
        }
    }
    return (color / totalWeight);
}


float4 main(VS_OUTPUT input) : SV_TARGET
{
    float3 norm = normalize(input.localPos);

    return float4(prefilterEnvMap(norm, roughness), 1.0f);
}
