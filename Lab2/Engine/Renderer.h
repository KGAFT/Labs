#pragma once
#include "../DXDevice/DXSwapChain.h"
#include "../DXDevice/DXDevice.h"
#include "../DXShader/Shader.h"
#include "../DXShader/ConstantBuffer.h"
#include "Camera/Camera.h"

struct ShaderConstant {
	DirectX::XMMATRIX worldMatrix;
	DirectX::XMMATRIX cameraMatrix;
};

class Renderer
{
private:
	static DXSwapChain* swapChain;
	static void resizeCallback(uint32_t width, uint32_t height);
public:
	Renderer(Window* window);
private:
	Window* engineWindow;
	DXDevice device;
	Shader* shader;
	ShaderConstant shaderConstant;
	ConstantBuffer* constantBuffer;
	VertexBuffer* cubeVertex = nullptr;
	IndexBuffer* cubeIndex = nullptr;
	Camera camera;
public:
	void drawFrame();
	void release();
private:
	void loadShader();
	void loadCube();
	void loadConstants();
};

