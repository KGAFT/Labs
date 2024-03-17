struct PixelShaderInput
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

cbuffer dataR: register(b0)
{
	matrix mWorldViewProj;
};

float4 main(PixelShaderInput psInput) : SV_Target
{
	   float temp[4] = (float[4])psInput.color.rgba;
	   temp[0] *= mWorldViewProj._11;
	   return (float4)temp;
}