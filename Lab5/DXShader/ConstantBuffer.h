#pragma once

#include <cstdint>
#include <d3d11.h>

class ConstantBuffer
{
public:
	ConstantBuffer(ID3D11Device* device, void* initData, size_t initDataSize, const char* bufferName = nullptr);
private:
	ID3D11Buffer* buffer;
public:
	void updateData(ID3D11DeviceContext* context, void* newData);
	void bindToVertexShader(ID3D11DeviceContext* context, uint32_t slot = 0);
	void bindToPixelShader(ID3D11DeviceContext* context, uint32_t slot = 0);
	~ConstantBuffer();
};

