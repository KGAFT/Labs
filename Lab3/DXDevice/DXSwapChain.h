#pragma once

#include <d3d11.h>
#include "DXRenderTargetView.h"

#define DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT 4

class DXSwapChain
{
	friend class DXDevice;
private:
	DXSwapChain(IDXGIFactory* factory, IDXGISwapChain* pSwapChain, ID3D11Device* device, HWND windowHandle, uint32_t width, uint32_t height, const char* name = nullptr);
private:
	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	DXRenderTargetView* rtv;
	IDXGIFactory* factory;
	std::vector<ID3D11Texture2D*> swapChainTextures;
	HWND windowHandle;
	uint32_t curImageCounter = 0;
	const char* name;
public:
	void resize(uint32_t width, uint32_t height);
	void bind(ID3D11DeviceContext* context, uint32_t width, uint32_t height);
	void clearRenderTargets(ID3D11DeviceContext* context, float r, float g, float b, float a);
	void present(bool vsync);
	uint32_t getCurrentImage() const;

	~DXSwapChain();
private:
	void destroyForRecreate();
	void destroy();
};

