
struct VS_INPUT
{
    float3 position: POSITION;
    float4 color: COLOR;
};

struct VS_OUTPUT
{
    float4 position: SV_POSITION;
    float4 color: COLOR;
};



VS_OUTPUT main(VS_INPUT vsInput)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.position = float4(vsInput.position, 1.0f);
    output.color = vsInput.color;
    return output;
}