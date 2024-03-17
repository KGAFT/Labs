struct PixelShaderInput
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

cbuffer constant: register(b0)
{
	float1 color;
};

float4 main(PixelShaderInput psInput) : SV_Target
{
	   return float4(color, 0, 0, 1);
}