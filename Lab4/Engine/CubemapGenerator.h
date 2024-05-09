#pragma once

#include "../DXShader/Shader.h"
#include "../DXDevice/DXDevice.h"

class CubemapGenerator
{
private:
    Shader* cubemapConvertShader = nullptr;
    Shader* irradianceGenerator = nullptr;
    DXDevice* device;

    HRESULT createCubemapTexture(UINT size, const std::string& key)
{
    ID3D11Texture2D* cubemapTexture = nullptr;
    ID3D11ShaderResourceView* cubemapSRV = nullptr;
    HRESULT result;
    {
        D3D11_TEXTURE2D_DESC textureDesc = {};

        textureDesc.Width = size;
        textureDesc.Height = size;
        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 6;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;
        result = device_->CreateTexture2D(&textureDesc, 0, &cubemapTexture);
    }

    if (SUCCEEDED(result))
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = 0;
        // Only create a view to one array element.
        rtvDesc.Texture2DArray.ArraySize = 1;
        for (int i = 0; i < 6 && SUCCEEDED(result); ++i)
        {
            // Create a render target view to the ith element.
            rtvDesc.Texture2DArray.FirstArraySlice = i;
            result = device_->CreateRenderTargetView(cubemapTexture, &rtvDesc, &cubemapRTV[i]);
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
        shaderResourceViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        result = device_->CreateShaderResourceView(cubemapTexture, &shaderResourceViewDesc, &cubemapSRV);
    }

    if (SUCCEEDED(result))
    {
        textureManager_.setDevice(device_);
        textureManager_.setDeviceContext(deviceContext_);
        result = textureManager_.loadTexture(cubemapTexture, cubemapSRV, key);
    }

    return result;
}

HRESULT renderToCubeMap(UINT size, int num)
{
    deviceContext_->OMSetRenderTargets(1, &cubemapRTV[num], nullptr);

    std::shared_ptr<ID3D11PixelShader> ps;
    std::shared_ptr<ID3D11VertexShader> vs;
    HRESULT result = PSManager_.get("copyToCubemapPS", ps);
    if (!SUCCEEDED(result))
        return result;
    result = VSManager_.get("mapping", vs);
    if (!SUCCEEDED(result))
        return result;

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = size;
    viewport.Height = size;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext_->RSSetViewports(1, &viewport);

    std::shared_ptr<ID3D11SamplerState> sampler;
    result = samplerManager_.get("default", sampler);
    if (!SUCCEEDED(result))
        return result;

    ID3D11SamplerState* samplers[] = {
        sampler.get()
    };
    deviceContext_->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = {
        subSRV
    };
    deviceContext_->PSSetShaderResources(0, 1, resources);
    deviceContext_->OMSetDepthStencilState(nullptr, 0);
    deviceContext_->RSSetState(nullptr);
    deviceContext_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    deviceContext_->IASetInputLayout(nullptr);
    deviceContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext_->VSSetShader(vs.get(), nullptr, 0);
    deviceContext_->PSSetShader(ps.get(), nullptr, 0);
    deviceContext_->Draw(6, 0);

    resources[0] = nullptr;
    deviceContext_->PSSetShaderResources(0, 1, resources);

    return result;
}
};
