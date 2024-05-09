#pragma once

#include <d3d11.h>
#include <cstdint>
#include <stdexcept>
#include "DXSwapChain.h"
#include "../Window/Window.h"
#include <map>

class DXDevice
{
public:
	DXDevice();
private:
	ID3D11Device* device = nullptr;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11DeviceContext* deviceContext = nullptr;
	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIAdapter* dxgiAdapter = nullptr;
	IDXGIFactory* dxgiFactory = nullptr;
	std::map<Window*, DXSwapChain*> swapChains;
public:
	DXSwapChain* getSwapChain(Window* window, const char* possibleName = nullptr);
	ID3D11DeviceContext* getDeviceContext();
	ID3D11Device* getDevice();
private:
	DXSwapChain* createSwapChain(Window* window, const char* possibleName = nullptr);
	void initializeDxgi();
public:
	~DXDevice();
};

