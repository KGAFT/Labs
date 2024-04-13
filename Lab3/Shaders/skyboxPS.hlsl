TextureCube cubeMap : register (t0);
SamplerState cubeMapSampler : register (s0);

struct PS_INPUT
{
    float4 position: SV_POSITION;
    float3 normal: NORMAL;
    float3 uv: UV;
};

float4 main(PS_INPUT input) : SV_TARGET {
    return float4(cubeMap.Sample(cubeMapSampler, input.uv).xyz, 1.0f)*100;
}