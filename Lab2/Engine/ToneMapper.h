#pragma once

#include <chrono>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <d3dcompiler.h>
#include <stdexcept>

#include "../DXDevice/DXRenderTargetView.h"
#include "../DXShader/ConstantBuffer.h"

struct Texture
{
    ID3D11Texture2D* texture = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11ShaderResourceView* shaderResourceView = nullptr;
};

struct ScaledFrame
{
    Texture min;
    Texture avg;
    Texture max;
};

struct AdaptData
{
    DirectX::XMFLOAT4 adapt;
};

class ToneMapper
{
private:
    ID3D11Device* device;
    DXRenderTargetView* rtv;
    uint32_t scaledTexturesAmount = 0;
    std::vector<ScaledFrame> scaledFrames;
    ID3D11SamplerState* samplerAvg;
    ID3D11SamplerState* samplerMin;
    ID3D11SamplerState* samplerMax;
    ConstantBuffer* constantBuffer;
    AdaptData adaptData{};

    ID3D11VertexShader* mappingVS;
    ID3D11PixelShader* brightnessPS;
    ID3D11PixelShader* downsamplePS;
    ID3D11PixelShader* tonemapPS;
    std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
    ID3D11Texture2D* readAvgTexture;
    float adapt = 0;
    float s = 0.5f;
public:

    void initialize(ID3D11Device* device, uint32_t width, uint32_t height)
    {
        createTextures(device, width, height);
        HRESULT result = 0;

        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MinLOD = -FLT_MAX;
        desc.MaxLOD = FLT_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;
        result = device->CreateSamplerState(&desc, &samplerAvg);


        if (SUCCEEDED(result))
        {
            desc.Filter = D3D11_FILTER_MINIMUM_ANISOTROPIC;

            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.MinLOD = -FLT_MAX;
            desc.MaxLOD = FLT_MAX;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 16;
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;
            result = device->CreateSamplerState(&desc, &samplerMin);
        }

        if (SUCCEEDED(result))
        {
            desc.Filter = D3D11_FILTER_MAXIMUM_ANISOTROPIC;

            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.MinLOD = -FLT_MAX;
            desc.MaxLOD = FLT_MAX;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 16;
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;
            result = device->CreateSamplerState(&desc, &samplerMax);
        }

        if (SUCCEEDED(result))
        {
            adaptData.adapt = DirectX::XMFLOAT4(0.0f, 0.5f, 0.0f, 0.0f);
            constantBuffer = new ConstantBuffer(device, &adaptData, sizeof(AdaptData), "Adapt data");
        }

        ID3DBlob* vertexShaderBuffer = nullptr;
        ID3DBlob* pixelShaderBuffer = nullptr;
        int flags = 0;
#ifdef _DEBUG // Включение/отключение отладочной информации для шейдеров в зависимости от конфигурации сборки
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        if (SUCCEEDED(result))
        {
            result = D3DCompileFromFile(L"Shaders/brightnessPS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0,
                                        &pixelShaderBuffer, NULL);
            if (SUCCEEDED(result))
            {
                result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
                                                   pixelShaderBuffer->GetBufferSize(), NULL, &brightnessPS);
            }
        }

        pixelShaderBuffer->Release();

        if (SUCCEEDED(result))
        {
            result = D3DCompileFromFile(L"Shaders/mappingVS.hlsl", NULL, NULL, "main", "vs_5_0", flags, 0, &vertexShaderBuffer,
                                        NULL);
            if (SUCCEEDED(result))
            {
                result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
                                                    vertexShaderBuffer->GetBufferSize(), NULL, &mappingVS);
            }
        }
        if (SUCCEEDED(result))
        {
            result = D3DCompileFromFile(L"Shaders/downsamplePS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0,
                                        &pixelShaderBuffer, NULL);
            if (SUCCEEDED(result))
            {
                result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
                                                   pixelShaderBuffer->GetBufferSize(), NULL, &downsamplePS);
            }
        }

        vertexShaderBuffer->Release();
        pixelShaderBuffer->Release();
        if (SUCCEEDED(result))
        {
            result = D3DCompileFromFile(L"Shaders/toneMapPS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer,
                                        NULL);
            if (SUCCEEDED(result))
            {
                result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
                                                   pixelShaderBuffer->GetBufferSize(), NULL, &tonemapPS);
            }
        }

        pixelShaderBuffer->Release();
        if(FAILED(result))
        {
            throw std::runtime_error("Failed to initialize shaders");
        }
    }

    void renderBrightness(ID3D11DeviceContext* deviceContext)
    {
        for (int i = scaledTexturesAmount; i >= 0; i--)
        {
            if (i == scaledTexturesAmount)
            {
                ID3D11RenderTargetView* views[] = { scaledFrames[i].avg.renderTargetView, 
                    scaledFrames[i].min.renderTargetView, 
                    scaledFrames[i].max.renderTargetView
                };
                deviceContext->OMSetRenderTargets(3, views, nullptr);
            }
            else
            {
                ID3D11RenderTargetView* views[] = { scaledFrames[i].avg.renderTargetView, scaledFrames[i].min.renderTargetView, scaledFrames[i].max.renderTargetView };
                deviceContext->OMSetRenderTargets(3, views, nullptr);
            }
		
            ID3D11SamplerState* samplers[] = { samplerAvg, samplerMin, samplerMax };
            deviceContext->PSSetSamplers(0, 3, samplers);

            D3D11_VIEWPORT viewport;
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = (FLOAT)pow(2, i);
            viewport.Height = (FLOAT)pow(2, i);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            deviceContext->RSSetViewports(1, &viewport);

            ID3D11ShaderResourceView* resources[] = { i == scaledTexturesAmount ? rtv->getResourceViews()[0] : scaledFrames[i + 1].avg.shaderResourceView,
                i == scaledTexturesAmount ? rtv->getResourceViews()[0] : scaledFrames[i + 1].min.shaderResourceView,
                i == scaledTexturesAmount ? rtv->getResourceViews()[0] : scaledFrames[i + 1].max.shaderResourceView 
            };
            deviceContext->PSSetShaderResources(0, 3, resources);
            deviceContext->OMSetDepthStencilState(nullptr, 0);
            deviceContext->RSSetState(nullptr);
            deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            deviceContext->IASetInputLayout(nullptr);
            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            deviceContext->VSSetShader(mappingVS, nullptr, 0);
            deviceContext->PSSetShader(i == scaledTexturesAmount ? brightnessPS : downsamplePS, nullptr, 0);
            deviceContext->Draw(6, 0);
        }
    }
    void RenderTonemap(ID3D11DeviceContext* deviceContext)
    {
        auto time = std::chrono::high_resolution_clock::now();
        float dtime = std::chrono::duration<float, std::milli>(time - lastFrameTime).count() * 0.001;
        lastFrameTime = time;

        deviceContext->CopyResource(readAvgTexture, scaledFrames[0].avg.texture);
        D3D11_MAPPED_SUBRESOURCE ResourceDesc = {};
        HRESULT hr = deviceContext->Map(readAvgTexture, 0, D3D11_MAP_READ, 0, &ResourceDesc);

        float avg;
        if (ResourceDesc.pData)
        {
            float* pData = reinterpret_cast<float*>(ResourceDesc.pData);
            avg = pData[0];
        }
        deviceContext->Unmap(readAvgTexture, 0);

        adapt += (avg - adapt) * (1.0f - exp(-dtime / s));

        
        adaptData.adapt = DirectX::XMFLOAT4(adapt, 0.0f, 0.0f, 0.0f);

        constantBuffer->updateData(deviceContext, &adaptData);

        ID3D11ShaderResourceView* resources[] = { rtv->getResourceViews()[0], 
            scaledFrames[0].avg.shaderResourceView,
                scaledFrames[0].min.shaderResourceView,
                scaledFrames[0].max.shaderResourceView
        };
        deviceContext->PSSetShaderResources(0, 4, resources);
        deviceContext->OMSetDepthStencilState(nullptr, 0);
        deviceContext->RSSetState(nullptr);
        deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        deviceContext->IASetInputLayout(nullptr);
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        constantBuffer->bindToPixelShader(deviceContext);
        deviceContext->VSSetShader(mappingVS, nullptr, 0);
        deviceContext->PSSetShader(tonemapPS, nullptr, 0);
        deviceContext->Draw(6, 0);
    }


    DXRenderTargetView* getRendertargetView()
    {
        return rtv;
    }

    void clearRenderTarget(ID3D11DeviceContext* deviceContext)
    {
        rtv->clearColorAttachments(deviceContext, 0.25f, 0.25f, 0.25f, 1.0f, 0);
        float color[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
        for (auto& scaledFrame : scaledFrames)
        {
            deviceContext->OMSetRenderTargets(1, &scaledFrame.avg.renderTargetView, NULL);
            deviceContext->ClearRenderTargetView(scaledFrame.avg.renderTargetView, color);
        }
    }
private:
    void createTextures(ID3D11Device* device, uint32_t width, uint32_t height)
    {
        rtv = new DXRenderTargetView(device, 1, width, height);
        int minSide = min(width, height);

        while (minSide >>= 1)
        {
            scaledTexturesAmount++;
        }

        for (int i = 0; i < scaledTexturesAmount + 1; i++)
        {
            ScaledFrame scaledFrame;
            createSquareTexture(device, scaledFrame.avg, i);
            createSquareTexture(device, scaledFrame.min, i);
            createSquareTexture(device, scaledFrame.max, i);
            scaledFrames.push_back(scaledFrame);
        }
        D3D11_TEXTURE2D_DESC textureDesc = {};

        textureDesc.Width = 1;
        textureDesc.Height = 1;
        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        textureDesc.MiscFlags = 0;

        device->CreateTexture2D(&textureDesc, NULL, &readAvgTexture);
    }

    void createSquareTexture(ID3D11Device* device, Texture& text, uint32_t len)
    {
        D3D11_TEXTURE2D_DESC desc;
        desc.Format = DXGI_FORMAT_R32_FLOAT;
        desc.ArraySize = 1;
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = pow(2, len);
        desc.Width = pow(2, len);
        HRESULT result = device->CreateTexture2D(&desc, nullptr, &text.texture);
        if (SUCCEEDED(result))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Format = DXGI_FORMAT_R32_FLOAT;
            descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            descSRV.Texture2D.MipLevels = 1;
            descSRV.Texture2D.MostDetailedMip = 0;
            result = device->CreateShaderResourceView(text.texture, &descSRV, &text.shaderResourceView);
            if (FAILED(result))
            {
                throw std::runtime_error("Failed to create shader resource view");
            }
        }
        else
        {
            throw std::runtime_error("Failed to create texture");
        }


        result = device->CreateRenderTargetView(text.texture, nullptr, &text.renderTargetView);
        if (FAILED(result))
        {
            throw std::runtime_error("Failed to create render target view");
        }
    }
};
