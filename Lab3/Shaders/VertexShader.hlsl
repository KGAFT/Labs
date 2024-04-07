
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
    float2 uv: UV;
    float4 color: COLOR;
};

cbuffer TransformData: register(b0)
{
    float4x4 worldMatrix;
    float4x4 cameraMatrix;
};


VS_OUTPUT main(VS_INPUT vsInput)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.position = mul(cameraMatrix, mul(worldMatrix, float4(vsInput.position, 1.0f)));
    output.uv = vsInput.uv;
    output.normal = vsInput.normal;
    output.color = vsInput.color;
    return output;
}