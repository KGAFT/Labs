Texture2D colorTexture : register (t0);
SamplerState colorSampler : register (s0);

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 localPos : POSITION;
};

float4 main(VS_OUTPUT input) : SV_TARGET{
    float3 pos = normalize(input.localPos);
    float PI = 3.14159265359f;
    float u = 1 - atan2(pos.z , pos.x) / (2 * PI);
    float v = -atan2(pos.y, sqrt(pos.x * pos.x + pos.z * pos.z)) / PI + 0.5f;

    return float4(colorTexture.Sample(colorSampler, float2(u, v)).xyz, 1.0f);
}