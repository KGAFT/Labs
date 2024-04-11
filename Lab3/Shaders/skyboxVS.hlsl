struct VS_INPUT
{
    float3 position: POSITION;
    float2 uv: UV;
    float3 normal: NORMAL;
    float4 color: COLOR;
};

struct VS_OUTPUT
{
    float4 position: SV_POSITION;
    float3 normal: NORMAL;
    float3 uv: UV;
};

cbuffer TransformData: register(b0)
{
    float4x4 worldMatrix;
    float4x4 cameraMatrix;
    float4 size;
    float3 cameraPosition;
};


VS_OUTPUT main(VS_INPUT vsInput)
{
    VS_OUTPUT output;

    float3 pos = cameraPosition.xyz + vsInput.position * size.x;
    output.position = mul(cameraPosition, mul(worldMatrix, float4(pos, 1.0f)));
    output.position.z =  0.0f;
    output.uv = vsInput.position;
    output.normal = vsInput.normal;
    return output;
}