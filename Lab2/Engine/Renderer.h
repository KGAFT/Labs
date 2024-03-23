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

struct PointLightSource {
	XMFLOAT3 position;
	float intensity = 1;
};
struct LightConstant {
	PointLightSource sources[3];
	XMFLOAT3 cameraPosition;
};

class Renderer : public IWindowKeyCallback
{
private:
	static DXSwapChain* swapChain;
	static void resizeCallback(uint32_t width, uint32_t height);
public:
	Renderer(Window* window);
private:
	Window* engineWindow;
	DXDevice device;
	std::vector<WindowKey> keys;
	Shader* shader;
	ShaderConstant shaderConstant{};
	LightConstant lightConstantData{};
	ConstantBuffer* constantBuffer;
	ConstantBuffer* lightConstant;
	VertexBuffer* cubeVertex = nullptr;
	IndexBuffer* cubeIndex = nullptr;
	Camera camera;
public:
	void drawFrame();
	void release();
	void keyEvent(WindowKey key) override;
	WindowKey* getKeys(uint32_t* pKeysAmountOut) override;
private:
	void loadShader();
	void loadCube();
	void loadConstants();
};
