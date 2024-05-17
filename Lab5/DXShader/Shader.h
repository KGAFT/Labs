#pragma once

#include <d3dcompiler.h>
#include <d3d11.h>
#include <cstdint>
#include <vector>
#include "VertexBuffer.h"
#include "IndexBuffer.h"

enum ShaderType {
	VERTEX_SHADER,
	PIXEL_SHADER
};

struct ShaderCreateInfo {
	const wchar_t* pathToShader;
	ShaderType shaderType;
	const char* shaderName = nullptr;
};


struct ShaderVertexInput {
	const char* shaderVariableName;
	uint32_t variableIndex;
	size_t vertexSize;
	DXGI_FORMAT inputFormat;
};

class Shader
{
public:
	static Shader* loadShader(ID3D11Device* device, ShaderCreateInfo* pCreateInfos, uint32_t shaderAmount);
public:
	Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3DBlob* vertexShaderData, ID3DBlob* pixelShaderData);
private:
	ID3D11PixelShader* pixelShader;
	ID3D11VertexShader* vertexShader;
	std::vector<D3D11_INPUT_ELEMENT_DESC> shaderInputs;
	ID3DBlob* vertexShaderData;
	ID3DBlob* pixelShaderData;
	ID3D11InputLayout* inputLayout;
public:
	void makeInputLayout(ID3D11Device* device, ShaderVertexInput* pInputs, uint32_t inputsAmount);
	void bind(ID3D11DeviceContext* deviceContext);
	void draw(ID3D11DeviceContext* context, IndexBuffer* indexBuffer, VertexBuffer* vertexBuffer);
	~Shader();
};

