float3 makeHDR(float3 baseColor, float exposure) {
	float gamma = 2.2;
	float3 mapped = float3(1, 1, 1) - exp(-baseColor * exposure);
	mapped = pow(mapped, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));
	return mapped;
}