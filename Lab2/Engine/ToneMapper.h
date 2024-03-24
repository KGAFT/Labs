﻿#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <d3dcompiler.h>

struct Texture
{
    ID3D11Texture2D* texture = nullptr;
    ID3D11RenderTargetView* RTV = nullptr;
    ID3D11ShaderResourceView* SRV = nullptr;
};

struct ScaledFrame
{
    Texture avg;
    Texture min;
    Texture max;
};

struct AdaptBuffer {
    DirectX::XMFLOAT4 adapt;
};

class ToneMapper
{
public:
private:
    ID3D11Texture2D* m_frameTexture;
    ID3D11RenderTargetView* m_frameRTV;
    ID3D11ShaderResourceView* m_frameSRV;
    int n = 0;

    ID3D11Texture2D* m_readAvgTexture;

    ID3D11Buffer* m_adaptBuffer;

    ID3D11SamplerState* m_sampler_avg;
    ID3D11SamplerState* m_sampler_min;
    ID3D11SamplerState* m_sampler_max;

    ID3D11VertexShader* m_mappingVS;
    ID3D11PixelShader* m_brightnessPS;
    ID3D11PixelShader* m_downsamplePS;
    ID3D11PixelShader* m_toneMapPS;
    std::vector<ScaledFrame> m_scaledFrames;
private:
HRESULT шnit(ID3D11Device* device, int textureWidth, int textureHeight)
{
	HRESULT result = createTextures(device, textureWidth, textureHeight);

	if (SUCCEEDED(result))
	{
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
		result = device->CreateSamplerState(&desc, &m_sampler_avg);
	}

	if (SUCCEEDED(result))
	{
		D3D11_SAMPLER_DESC desc = {};
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
		result = device->CreateSamplerState(&desc, &m_sampler_min);
	}

	if (SUCCEEDED(result))
	{
		D3D11_SAMPLER_DESC desc = {};
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
		result = device->CreateSamplerState(&desc, &m_sampler_max);
	}

	if (SUCCEEDED(result)) {
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(AdaptBuffer);
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		AdaptBuffer adaptBuffer;
		adaptBuffer.adapt = DirectX::XMFLOAT4(0.0f, 0.5f, 0.0f, 0.0f);

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = &adaptBuffer;
		data.SysMemPitch = sizeof(adaptBuffer);
		data.SysMemSlicePitch = 0;

		result = device->CreateBuffer(&desc, &data, &m_adaptBuffer);
	}

	ID3D10Blob* vertexShaderBuffer = nullptr;
	ID3D10Blob* pixelShaderBuffer = nullptr;
	int flags = 0;
#ifdef _DEBUG 
	flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	if (SUCCEEDED(result)) {
		result = D3DCompileFromFile(L"brightnessPS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
		if (SUCCEEDED(result)) {
			result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_brightnessPS);
		}
	}

	pixelShaderBuffer->Release();

	if (SUCCEEDED(result)) {
		result = D3DCompileFromFile(L"mappingVS.hlsl", NULL, NULL, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
		if (SUCCEEDED(result)) {
			result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_mappingVS);
		}
	}
	if (SUCCEEDED(result)) {
		result = D3DCompileFromFile(L"downsamplePS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
		if (SUCCEEDED(result)) {
			result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_downsamplePS);
		}
	}

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	if (SUCCEEDED(result)) {
		result = D3DCompileFromFile(L"toneMapPS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
		if (SUCCEEDED(result)) {
			result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_toneMapPS);
		}
	}

	pixelShaderBuffer->Release();

	return result;
}
    
    HRESULT createTextures(ID3D11Device* device, int textureWidth, int textureHeight)
    {
        HRESULT result;
        {
            D3D11_TEXTURE2D_DESC textureDesc = {};

            textureDesc.Width = textureWidth;
            textureDesc.Height = textureHeight;
            textureDesc.MipLevels = 1;
            textureDesc.ArraySize = 1;
            textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            textureDesc.CPUAccessFlags = 0;
            textureDesc.MiscFlags = 0;

            result = device->CreateTexture2D(&textureDesc, NULL, &m_frameTexture);
        }

        if (SUCCEEDED(result))
        {
            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
            renderTargetViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            renderTargetViewDesc.Texture2D.MipSlice = 0;

            result = device->CreateRenderTargetView(m_frameTexture, &renderTargetViewDesc, &m_frameRTV);
        }

        if (SUCCEEDED(result))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
            shaderResourceViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
            shaderResourceViewDesc.Texture2D.MipLevels = 1;

            result = device->CreateShaderResourceView(m_frameTexture, &shaderResourceViewDesc, &m_frameSRV);
        }

        if (SUCCEEDED(result))
        {
            int minSide = min(textureWidth, textureHeight);

            while (minSide >>= 1)
            {
                n++;
            }

            for (int i = 0; i < n + 1; i++)
            {
                ScaledFrame scaledFrame;

                result = createTexture(device, scaledFrame.avg, i);
                if (!SUCCEEDED(result))
                    break;

                result = createTexture(device, scaledFrame.min, i);
                if (!SUCCEEDED(result))
                    break;

                result = createTexture(device, scaledFrame.max, i);
                if (!SUCCEEDED(result))
                    break;

                m_scaledFrames.push_back(scaledFrame);
            }
        }

        if (SUCCEEDED(result))
        {
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

            result = device->CreateTexture2D(&textureDesc, NULL, &m_readAvgTexture);
        }

        return result;
    }
    HRESULT createTexture(ID3D11Device* device, Texture& text, int num)
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
        desc.Height = pow(2, num);
        desc.Width = pow(2, num);
        HRESULT result = device->CreateTexture2D(&desc, nullptr, &text.texture);
        if (SUCCEEDED(result))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Format = DXGI_FORMAT_R32_FLOAT;
            descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            descSRV.Texture2D.MipLevels = 1;
            descSRV.Texture2D.MostDetailedMip = 0;
            result = device->CreateShaderResourceView(text.texture, &descSRV, &text.SRV);
        }

        if (SUCCEEDED(result))
        {
            result = device->CreateRenderTargetView(text.texture, nullptr, &text.RTV);
        }

        return result;
    }
};
