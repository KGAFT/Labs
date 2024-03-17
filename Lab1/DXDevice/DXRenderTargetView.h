#pragma once

#include <d3d11.h>
#include <cstdint>
#include <vector>

class DXRenderTargetView
{
public:
	DXRenderTargetView(ID3D11Device* device, uint32_t colorAttachmentCount, uint32_t width, uint32_t height);
	DXRenderTargetView(ID3D11Device* device, std::vector <ID3D11Texture2D*> colorAttachment, uint32_t width, uint32_t height);
	DXRenderTargetView(ID3D11Device* device, std::vector <ID3D11Texture2D*> colorAttachment, ID3D11Texture2D* depthAttachment);
private:
	ID3D11Device* device = nullptr;
	std::vector <ID3D11Texture2D*> colorAttachments;
	ID3D11Texture2D* depthAttachment = nullptr;
	ID3D11DepthStencilView* depthView = nullptr;
	std::vector<ID3D11RenderTargetView*> renderTargetViews;
	D3D11_VIEWPORT vp = {};

	bool colorCreatedInside = false;
	bool depthCreatedInside = false;
public:
	void bind(ID3D11DeviceContext* context, uint32_t width, uint32_t height, int curImage);
	void clearColorAttachments(ID3D11DeviceContext* context, float r, float g, float b, float a, int currentImage);
	void clearDepthAttachments(ID3D11DeviceContext* context);
	void resize(uint32_t width, uint32_t height);
	void resize(std::vector <ID3D11Texture2D*> colorAttachment, uint32_t width, uint32_t height);
	void resize(std::vector <ID3D11Texture2D*> colorAttachment, ID3D11Texture2D* depthAttachment, uint32_t width, uint32_t height);
private:
	void createColorAttachment(uint32_t colorAttachmentCount, uint32_t width, uint32_t height);
	void createDepthAttachment(uint32_t width, uint32_t height);
	void createRenderTarget();
	void createDepthStencilView();
public:
	~DXRenderTargetView();
public:
	void destroy();
};

