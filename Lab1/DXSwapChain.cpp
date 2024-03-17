#include "DXSwapChain.h"
#include <stdexcept>

DXSwapChain::DXSwapChain(IDXGIFactory* factory, IDXGISwapChain* pSwapChain, ID3D11Device* device, HWND windowHandle, uint32_t width, uint32_t height) : 
	factory(factory), swapChain(pSwapChain), device(device), windowHandle(windowHandle){
	swapChainTextures.resize(DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT);
	for (uint32_t i = 0; i < DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT; i++) {
		if (FAILED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainTextures[i])))) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}
	}
	
	
	rtv = new DXRenderTargetView(device,  swapChainTextures, width, height);
}

void DXSwapChain::resize(uint32_t width, uint32_t height) {
	destroyForRecreate();
	swapChain->ResizeBuffers(DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	for (uint32_t i = 0; i < DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT; i++) {
		if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainTextures[i])))) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}
	}
	rtv->resize(swapChainTextures , width, height);
}

void DXSwapChain::bind(ID3D11DeviceContext* context, uint32_t width, uint32_t height) {
	rtv->bind(context, width, height, curImageCounter);
	curImageCounter++;
	if (curImageCounter >= DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT) {
		curImageCounter = 0;
	}
}
void DXSwapChain::clearRenderTargets(ID3D11DeviceContext* context, float r, float g, float b, float a) {
	rtv->clearColorAttachments(context, r, g, b, a);
	rtv->clearDepthAttachments(context);
}
void DXSwapChain::present(bool vsync) {
	swapChain->Present(vsync, 0);
}

DXSwapChain::~DXSwapChain() {
	destroy();
}

void DXSwapChain::destroyForRecreate() {
	rtv->destroy();
	for (auto& item : swapChainTextures) {
		item->Release();
	}
}
void DXSwapChain::destroy() {
	swapChain->Release();
	for (auto& item : swapChainTextures) {
		item->Release();
	}
	delete rtv;
}