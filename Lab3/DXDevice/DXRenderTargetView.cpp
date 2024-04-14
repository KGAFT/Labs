#include "DXRenderTargetView.h"
#include <sstream>
#include <stdexcept>

D3D11_TEXTURE2D_DESC depthTextureDesc{
    800, 600, 1, 1, DXGI_FORMAT_D24_UNORM_S8_UINT, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_DEPTH_STENCIL, 0, 0
};
D3D11_TEXTURE2D_DESC colorTextureDesc{
    800, 600, 1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 0
};
D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{depthTextureDesc.Format, D3D11_DSV_DIMENSION_TEXTURE2DMS, 0, 0};
FLOAT toClear[4] = {0, 0, 0, 1};

DXRenderTargetView::DXRenderTargetView(ID3D11Device* device, uint32_t colorAttachmentCount, uint32_t width,
                                       uint32_t height, const char* name) : device(device),
                                                                            vp(D3D11_VIEWPORT{
                                                                                (FLOAT)width, (FLOAT)height, 0.0, 1.0f
                                                                            }), depthCreatedInside(true),
                                                                            colorCreatedInside(true)
{
    createColorAttachment(colorAttachmentCount, width, height);
    createDepthAttachment(width, height);
    createRenderTarget(name);
    createDepthStencilView(name);
    createShaderResourceViews(name);
}

DXRenderTargetView::DXRenderTargetView(ID3D11Device* device, std::vector<ID3D11Texture2D*> colorAttachment,
                                       uint32_t width, uint32_t height, const char* name) : device(device),
    colorAttachments(colorAttachment), vp(D3D11_VIEWPORT{(FLOAT)width, (FLOAT)height, 0.0, 1.0f}),
    depthCreatedInside(true)
{
    createDepthAttachment(width, height);
    createRenderTarget(name);
    createDepthStencilView(name);
}

DXRenderTargetView::DXRenderTargetView(ID3D11Device* device, std::vector<ID3D11Texture2D*> colorAttachment,
                                       ID3D11Texture2D* depthAttachment, const char* name) : device(device),
    colorAttachments(colorAttachment), depthAttachment(depthAttachment), vp(D3D11_VIEWPORT{800.0f, 600.0f, 0.0, 1.0f})
{
    createRenderTarget(name);
    createDepthStencilView(name);
}

void DXRenderTargetView::clearColorAttachments(ID3D11DeviceContext* context, float r, float g, float b, float a,
                                               int currentImage)
{
    toClear[0] = r;
    toClear[1] = g;
    toClear[2] = b;
    toClear[3] = a;
    if (currentImage < 0)
    {
        for (auto& item : renderTargetViews)
        {
            context->ClearRenderTargetView(item, toClear);
        }
    }
    else
    {
        context->ClearRenderTargetView(renderTargetViews[currentImage], toClear);
    }
}

void DXRenderTargetView::clearDepthAttachments(ID3D11DeviceContext* context)
{
    context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DXRenderTargetView::bind(ID3D11DeviceContext* context, uint32_t width, uint32_t height, int curImage)
{
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    context->RSSetViewports(1, &vp);
    if (curImage < 0)
    {
        context->OMSetRenderTargets((UINT)renderTargetViews.size(), renderTargetViews.data(), depthView);
    }
    else
    {
        context->OMSetRenderTargets(1, &renderTargetViews[curImage], depthView);
    }
}


void DXRenderTargetView::createRenderTarget(const char* name)
{
    renderTargetViews.resize(colorAttachments.size());
    std::stringstream nameStream;
    for (uint32_t i = 0; i < colorAttachments.size(); i++)
    {
        if (FAILED(device->CreateRenderTargetView(colorAttachments[i], nullptr, &renderTargetViews[i])))
        {
            throw std::runtime_error("Failed to create render target view");
        }
#if defined(_DEBUG)
        if (name)
        {
            nameStream << name << " render target view " << i;
            std::string finalName = nameStream.str();
            nameStream.clear();
            nameStream.str("");
            colorAttachments[i]->SetPrivateData(WKPDID_D3DDebugObjectName, finalName.length() * sizeof(char),
                                                finalName.c_str());
        }
#endif
    }
}

void DXRenderTargetView::resize(uint32_t width, uint32_t height, const char* name)
{
    createColorAttachment(colorAttachments.size(), width, height);
    createDepthAttachment(width, height);
    createRenderTarget(name);
    createDepthStencilView(name);
    createShaderResourceViews(name);
}

void DXRenderTargetView::resize(std::vector<ID3D11Texture2D*> colorAttachment, uint32_t width, uint32_t height,
                                const char* name)
{
    DXRenderTargetView::colorAttachments = colorAttachment;
    createDepthAttachment(width, height);
    createRenderTarget(name);
    createDepthStencilView(name);
}

void DXRenderTargetView::resize(std::vector<ID3D11Texture2D*> colorAttachment, ID3D11Texture2D* depthAttachment,
                                uint32_t width, uint32_t height, const char* name)
{
    DXRenderTargetView::colorAttachments = colorAttachment;
    DXRenderTargetView::depthAttachment = depthAttachment;
    createRenderTarget(name);
    createDepthStencilView(name);
}

std::vector<ID3D11RenderTargetView*> DXRenderTargetView::getRenderTargetViews() const
{
    return renderTargetViews;
}

std::vector<ID3D11ShaderResourceView*> DXRenderTargetView::getResourceViews() const
{
    return resourceViews;
}

void DXRenderTargetView::createColorAttachment(uint32_t colorAttachmentCount, uint32_t width, uint32_t height)
{
    colorTextureDesc.Width = width;
    colorTextureDesc.Height = height;
    colorAttachments.resize(colorAttachmentCount);
    for (uint32_t i = 0; i < colorAttachmentCount; i++)
    {
        if (FAILED(device->CreateTexture2D(&colorTextureDesc, nullptr, &colorAttachments[i])))
        {
            throw std::runtime_error("Failed to create color attachment");
        }
    }
}

void DXRenderTargetView::createDepthAttachment(uint32_t width, uint32_t height)
{
    depthTextureDesc.Width = width;
    depthTextureDesc.Height = height;
    if (FAILED(device->CreateTexture2D(&depthTextureDesc, nullptr, &depthAttachment)))
    {
        throw std::runtime_error("Failed to create depth attachment");
    }
}

void DXRenderTargetView::createDepthStencilView(const char* name)
{
    if (FAILED(device->CreateDepthStencilView(depthAttachment, &dsvDesc, &depthView)))
    {
        throw std::runtime_error("Failed to create depth stencil view");
    }
#if defined(_DEBUG)
    if (name)
    {
        std::string finalName = name;
        finalName += " depth stencil view ";
        depthAttachment->SetPrivateData(WKPDID_D3DDebugObjectName, finalName.length() * sizeof(char),
                                        finalName.c_str());
    }
#endif
}

void DXRenderTargetView::createShaderResourceViews(const char* name)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
    descSRV.Format = colorTextureDesc.Format;
    descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    descSRV.Texture2D.MipLevels = 1;
    descSRV.Texture2D.MostDetailedMip = 0;
    ID3D11ShaderResourceView* pResourceView;
    std::string finalName;


    if (colorCreatedInside)
    {
        uint32_t i = 0;
        for (auto colorAttachment : colorAttachments)
        {
            if (FAILED(device->CreateShaderResourceView(colorAttachment, &descSRV, &pResourceView)))
            {
                throw std::runtime_error("Failed to create shader resource view");
            }
            if (finalName.length())
            {
                finalName = name;
                finalName+=" shader resource view "+i;
                pResourceView->SetPrivateData(WKPDID_D3DDebugObjectName, finalName.length() * sizeof(char),
                                              finalName.c_str());
            }
            resourceViews.push_back(pResourceView);
            i++;
        }
    }

}

DXRenderTargetView::~DXRenderTargetView()
{
    destroy();
}

void DXRenderTargetView::destroy()
{
    depthView->Release();
    for (auto resourceView : resourceViews)
    {
        resourceView->Release();
    }
    for (auto& item : renderTargetViews)
    {
        item->Release();
    }
    if (colorCreatedInside)
    {
        for (auto& item : colorAttachments)
        {
            item->Release();
        }
    }
    if (depthCreatedInside)
    {
        depthAttachment->Release();
    }
    resourceViews.clear();          
}
