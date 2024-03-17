#include "DXRenderTargetView.h"
#include <stdexcept>

D3D11_TEXTURE2D_DESC depthTextureDesc{800, 600, 1, 1,DXGI_FORMAT_D24_UNORM_S8_UINT, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_DEPTH_STENCIL,0,0};
D3D11_TEXTURE2D_DESC colorTextureDesc{800, 600, 1, 1,DXGI_FORMAT_R32G32B32A32_FLOAT, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET,0,0 };
D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{ depthTextureDesc.Format, D3D11_DSV_DIMENSION_TEXTURE2DMS, 0, 0 };
FLOAT toClear[4] = { 0,0,0,1 };

DXRenderTargetView::DXRenderTargetView(ID3D11Device* device, uint32_t colorAttachmentCount, uint32_t width, uint32_t height) : device(device),
vp(D3D11_VIEWPORT{ (FLOAT)width, (FLOAT)height, 0.0, 1.0f }), depthCreatedInside(true), colorCreatedInside(true) {
    createColorAttachment(colorAttachmentCount, width, height);
    createDepthAttachment(width, height);
    createRenderTarget();
    createDepthStencilView();
}
DXRenderTargetView::DXRenderTargetView(ID3D11Device* device, std::vector<ID3D11Texture2D*> colorAttachment, uint32_t width, uint32_t height) : device(device),
colorAttachments(colorAttachment), vp(D3D11_VIEWPORT{(FLOAT)width, (FLOAT)height, 0.0, 1.0f}), depthCreatedInside(true) {
    createDepthAttachment(width, height);
    createRenderTarget();
    createDepthStencilView();
}

DXRenderTargetView::DXRenderTargetView(ID3D11Device* device, std::vector <ID3D11Texture2D*> colorAttachment, ID3D11Texture2D* depthAttachment) : device(device),
colorAttachments(colorAttachment), depthAttachment(depthAttachment), vp(D3D11_VIEWPORT{ 800.0f, 600.0f, 0.0, 1.0f }) {
    createRenderTarget();
    createDepthStencilView();
}

void DXRenderTargetView::clearColorAttachments(ID3D11DeviceContext* context, float r, float g, float b, float a) {
    toClear[0] = r;
    toClear[1] = g;
    toClear[2] = b;
    toClear[3] = a;
    for (auto& item : renderTargetViews) {
        context->ClearRenderTargetView(item, toClear);
    }
}
void DXRenderTargetView::clearDepthAttachments(ID3D11DeviceContext* context) {
    context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DXRenderTargetView::bind(ID3D11DeviceContext* context, uint32_t width, uint32_t height, int curImage) {
    vp.Width = width;
    vp.Height = height;
    context->RSSetViewports(1, &vp);
    if (curImage < 0) {
        context->OMSetRenderTargets(renderTargetViews.size(), renderTargetViews.data(), depthView);
    }
    else {
        context->OMSetRenderTargets(1, &renderTargetViews[curImage], depthView);
    }
}


void DXRenderTargetView::createRenderTarget() {
    renderTargetViews.resize(colorAttachments.size());
    for (uint32_t i = 0; i < colorAttachments.size(); i++) {
        if (FAILED(device->CreateRenderTargetView(colorAttachments[i], nullptr, &renderTargetViews[i]))) {
            throw std::runtime_error("Failed to create render target view");
        }
    }
    
}

void DXRenderTargetView::resize(uint32_t width, uint32_t height) {
    createColorAttachment(colorAttachments.size(), width, height);
    createDepthAttachment(width, height);
    createRenderTarget();
    createDepthStencilView();
}
void DXRenderTargetView::resize(std::vector <ID3D11Texture2D*> colorAttachment, uint32_t width, uint32_t height) {
    DXRenderTargetView::colorAttachments = colorAttachment;
    createDepthAttachment(width, height);
    createRenderTarget();
    createDepthStencilView();
}
void DXRenderTargetView::resize(std::vector <ID3D11Texture2D*> colorAttachment, ID3D11Texture2D* depthAttachment, uint32_t width, uint32_t height) {
    DXRenderTargetView::colorAttachments = colorAttachment;
    DXRenderTargetView::depthAttachment = depthAttachment;
    createRenderTarget();
    createDepthStencilView();
}

void DXRenderTargetView::createColorAttachment(uint32_t colorAttachmentCount, uint32_t width, uint32_t height) {
    colorTextureDesc.Width = width;
    colorTextureDesc.Height = height;
    colorAttachments.resize(colorAttachmentCount);
    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        if (FAILED(device->CreateTexture2D(&colorTextureDesc, nullptr, &colorAttachments[i]))) {
            throw std::runtime_error("Failed to create color attachment");
        }
    }
    
}
void DXRenderTargetView::createDepthAttachment(uint32_t width, uint32_t height) {
    depthTextureDesc.Width = width;
    depthTextureDesc.Height = height;
    if (FAILED(device->CreateTexture2D(&depthTextureDesc, nullptr, &depthAttachment))) {
        throw std::runtime_error("Failed to create depth attachment");
    }
}

void DXRenderTargetView::createDepthStencilView() {
    if (FAILED(device->CreateDepthStencilView(depthAttachment, &dsvDesc, &depthView))) {
        throw std::runtime_error("Failed to create depth stencil view");
    }
}

DXRenderTargetView::~DXRenderTargetView() {
    destroy();
}

void DXRenderTargetView::destroy() {
    depthView->Release();
    for (auto& item : renderTargetViews) {
        item->Release();
    }
    if (colorCreatedInside) {
        for (auto& item : colorAttachments) {
            item->Release();
        }
    }
    if (depthCreatedInside) {
        depthAttachment->Release();
    }
}