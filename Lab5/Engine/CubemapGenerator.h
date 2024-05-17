#pragma once

#include "../DXShader/Shader.h"
#include "../DXDevice/DXDevice.h"
#include "../Utils/FileSystemUtils.h"
#include "../STB/stb_image.h"

struct HDRCubemap
{
    ID3D11Texture2D* sourceTexture;
    ID3D11ShaderResourceView* sourceResourceView;
    ID3D11Texture2D* cubemapTexture;
    ID3D11ShaderResourceView* cubemapSRV;

    ID3D11Texture2D* irradianceTexture;
    ID3D11ShaderResourceView* irradianceSRV;

    ID3D11Texture2D* prefilteredTexture;
    ID3D11ShaderResourceView* prefilteredSRV;
};

struct Quad
{
    VertexBuffer* quadMeshVertex;
    IndexBuffer* quadMeshIndex;
    
};

struct ViewMat
{
    XMMATRIX viewProjMatrix;
};

class CubemapGenerator
{
public:
    CubemapGenerator(DXDevice* device)
        : device(device)
    {
        loadShaders();
        loadQuad();
        viewMatrices = {
            DirectX::XMMatrixLookToLH(
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            ),
            DirectX::XMMatrixLookToLH(
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            ),
            DirectX::XMMatrixLookToLH(
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)
            ),
            DirectX::XMMatrixLookToLH(
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)
            ),
            DirectX::XMMatrixLookToLH(
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            ),
            DirectX::XMMatrixLookToLH(
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f),
                DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            ),
        };
        viewProjMatrixBuff = new ConstantBuffer(device->getDevice(), &data, sizeof(ViewMat));

        D3D11_SAMPLER_DESC desc = {};
        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MinLOD = -FLT_MAX;
        desc.MaxLOD = FLT_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;
        if(FAILED(device->getDevice()->CreateSamplerState(&desc, &sampler)))
        {
            throw std::runtime_error("Failed to load sampler");
        }
    }

private:
    Shader* cubemapConvertShader = nullptr;
    Shader* irradianceGenerator = nullptr;
    Shader* prefilterShader = nullptr;
    DXDevice* device;


    std::vector<Quad> quads;

    ID3D11SamplerState* sampler;
    std::vector<XMMATRIX> viewMatrices;
    ConstantBuffer* viewProjMatrixBuff = nullptr;
    ViewMat data{};
    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PI / 2, 1.0f, 0.1f, 10.0f);
public:
    void loadHDRCubemap(std::string name, HDRCubemap* pOutput)
    {
        uint32_t sideSize = 0;
        uint32_t irradianceSideSize = 0;
        loadHDRMap(name, &sideSize, &pOutput->sourceTexture, &pOutput->sourceResourceView);
        irradianceSideSize = 32;
        createCubemap(pOutput, sideSize, irradianceSideSize);
        DXRenderTargetView* rtv = new DXRenderTargetView(device->getDevice(), pOutput->cubemapTexture, sideSize,
                                                         sideSize, 6, "Cube rendertarget view");
        DXRenderTargetView* irradianceRTV = new DXRenderTargetView(device->getDevice(), pOutput->irradianceTexture, sideSize,
                                                        sideSize, 6, "Cube rendertarget view");
        renderCube(rtv, pOutput->sourceResourceView, sideSize);
        renderIrradianceCube(irradianceRTV, pOutput->cubemapSRV, irradianceSideSize);
        rtv->destroy();
        irradianceRTV->destroy();
    }

private:
    void renderCube(DXRenderTargetView* cubeRenderTargetView, ID3D11ShaderResourceView* pSourceResourceView,
                    uint32_t sideSize)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            cubeRenderTargetView->clearColorAttachments(device->getDeviceContext(), 0.25, 0.25, 0.25, 1.0, i);
            cubeRenderTargetView->bind(device->getDeviceContext(), sideSize, sideSize, i, false);
            cubemapConvertShader->bind(device->getDeviceContext());
            device->getDeviceContext()->PSSetShaderResources(0, 1, &pSourceResourceView);
            device->getDeviceContext()->PSSetSamplers(0, 1, &sampler);
            device->getDeviceContext()->OMSetDepthStencilState(nullptr, 0);
            device->getDeviceContext()->RSSetState(nullptr);
            device->getDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            data.viewProjMatrix = XMMatrixMultiply(viewMatrices[i], projectionMatrix);
            viewProjMatrixBuff->updateData(device->getDeviceContext(), &data);
            viewProjMatrixBuff->bindToVertexShader(device->getDeviceContext());
            cubemapConvertShader->draw(device->getDeviceContext(), quads[i].quadMeshIndex, quads[i].quadMeshVertex);
        }
        DXDevice::unBindRenderTargets(device->getDeviceContext());
    }

    void renderIrradianceCube(DXRenderTargetView* cubeRenderTargetView, ID3D11ShaderResourceView* pSourceResourceView,
                    uint32_t sideSize)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            cubeRenderTargetView->clearColorAttachments(device->getDeviceContext(), 0.25, 0.25, 0.25, 1.0, i);
            cubeRenderTargetView->bind(device->getDeviceContext(), sideSize, sideSize, i, false);
            irradianceGenerator->bind(device->getDeviceContext());
            device->getDeviceContext()->PSSetShaderResources(0, 1, &pSourceResourceView);
            device->getDeviceContext()->PSSetSamplers(0, 1, &sampler);
            device->getDeviceContext()->OMSetDepthStencilState(nullptr, 0);
            device->getDeviceContext()->RSSetState(nullptr);
            device->getDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            data.viewProjMatrix = XMMatrixMultiply(viewMatrices[i], projectionMatrix);
            viewProjMatrixBuff->updateData(device->getDeviceContext(), &data);
            viewProjMatrixBuff->bindToVertexShader(device->getDeviceContext());
            irradianceGenerator->draw(device->getDeviceContext(), quads[i].quadMeshIndex, quads[i].quadMeshVertex);
        }
        DXDevice::unBindRenderTargets(device->getDeviceContext());
    }

    void renderPrefilterMap(DXRenderTargetView* cubeRenderTargetView, ID3D11ShaderResourceView* pSourceResourceView,
                    uint32_t sideSize)
    {
        for (uint32_t i = 0; i < 6; i++)
        {
            cubeRenderTargetView->clearColorAttachments(device->getDeviceContext(), 0.25, 0.25, 0.25, 1.0, i);
            cubeRenderTargetView->bind(device->getDeviceContext(), sideSize, sideSize, i, false);
            prefilterShader->bind(device->getDeviceContext());
            device->getDeviceContext()->PSSetShaderResources(0, 1, &pSourceResourceView);
            device->getDeviceContext()->PSSetSamplers(0, 1, &sampler);
            device->getDeviceContext()->OMSetDepthStencilState(nullptr, 0);
            device->getDeviceContext()->RSSetState(nullptr);
            device->getDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            data.viewProjMatrix = XMMatrixMultiply(viewMatrices[i], projectionMatrix);
            viewProjMatrixBuff->updateData(device->getDeviceContext(), &data);
            viewProjMatrixBuff->bindToVertexShader(device->getDeviceContext());
            prefilterShader->draw(device->getDeviceContext(), quads[i].quadMeshIndex, quads[i].quadMeshVertex);
        }
        DXDevice::unBindRenderTargets(device->getDeviceContext());
    }
    
    void createCubemap(HDRCubemap* pOutput, uint32_t size, uint32_t irradianceSideSize)
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
        if (FAILED(device->getDevice()->CreateTexture2D(&textureDesc, 0, &pOutput->cubemapTexture)))
        {
            throw std::runtime_error("Failed to create resulting cubemap texture");
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
        shaderResourceViewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        if(FAILED(device->getDevice()->CreateShaderResourceView(pOutput->cubemapTexture, &shaderResourceViewDesc,
                                                      &pOutput->cubemapSRV)))
        {
            throw std::runtime_error("Failed to create shader resource view cubemap");
        }
        textureDesc.Width = irradianceSideSize;
        textureDesc.Height = irradianceSideSize;

        if (FAILED(device->getDevice()->CreateTexture2D(&textureDesc, 0, &pOutput->irradianceTexture)))
        {
            throw std::runtime_error("Failed to create resulting cubemap texture");
        }
        if(FAILED(device->getDevice()->CreateShaderResourceView(pOutput->irradianceTexture, &shaderResourceViewDesc,
                                                      &pOutput->irradianceSRV)))
        {
            throw std::runtime_error("Failed to create shader resource view cubemap");
        }
    }

    void loadHDRMap(std::string name, uint32_t* pSizeOutput, ID3D11Texture2D** ppTextureResult,
                    ID3D11ShaderResourceView** ppResourceViewRes)
    {
        auto workDir = FileSystemUtils::getCurrentDirectoryPath();

        std::string filePath(workDir.begin(), workDir.end());
        filePath += name;
        int width, height, nrComponents;
        float* data = stbi_loadf(filePath.c_str(), &width, &height, &nrComponents, 4);

        if (!data)
        {
            throw std::runtime_error("Failed to load hdr");
        }

        *pSizeOutput = min(width, height);
        D3D11_TEXTURE2D_DESC textureDesc = {};

        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = data;
        initData.SysMemPitch = width * sizeof(float) * 4;
        initData.SysMemSlicePitch = width * height * sizeof(float) * 4;

        HRESULT result = device->getDevice()->CreateTexture2D(&textureDesc, &initData, ppTextureResult);

        if (SUCCEEDED(result))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            descSRV.Texture2D.MipLevels = 1;
            descSRV.Texture2D.MostDetailedMip = 0;
            result = device->getDevice()->CreateShaderResourceView(*ppTextureResult, &descSRV, ppResourceViewRes);
        }
        else
        {
            throw std::runtime_error("Failed to create cubemap texture");
        }
        stbi_image_free(data);
    }

    void loadShaders()
    {
        ShaderCreateInfo createInfos[2];
        createInfos[0].shaderName = "CubeSideVS";
        createInfos[0].pathToShader = L"Shaders/CubemapGen/CubeSideVS.hlsl";
        createInfos[0].shaderType = ShaderType::VERTEX_SHADER;

        createInfos[1].shaderName = "HDRToCube";
        createInfos[1].pathToShader = L"Shaders/CubemapGen/HDRToCubePS.hlsl";
        createInfos[1].shaderType = ShaderType::PIXEL_SHADER;
        cubemapConvertShader = Shader::loadShader(device->getDevice(), createInfos, 2);
        ShaderVertexInput vertexInputs[1];
        vertexInputs[0].inputFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        vertexInputs[0].variableIndex = 0;
        vertexInputs[0].vertexSize = sizeof(float) * 3;
        vertexInputs[0].shaderVariableName = "POSITION";
        cubemapConvertShader->makeInputLayout(device->getDevice(), vertexInputs, 1);
        createInfos[1].shaderName = "irradianceCube";
        createInfos[1].pathToShader = L"Shaders/CubemapGen/irradianceCube.hlsl";
        irradianceGenerator = Shader::loadShader(device->getDevice(), createInfos, 2);
        irradianceGenerator->makeInputLayout(device->getDevice(),  vertexInputs, 1);
        createInfos[1].shaderName = "prefilterer";
        createInfos[1].pathToShader = L"Shaders/CubemapGen/prefilterCube.hlsl";
        prefilterShader = Shader::loadShader(device->getDevice(), createInfos, 2);
        prefilterShader->makeInputLayout(device->getDevice(),  vertexInputs, 1);

    }

    void loadQuad()
    {
        quads.push_back({
            new VertexBuffer(device->getDevice(), sizeof(quadVerticesXPos),
                                                     sizeof(float) * 3, quadVerticesXPos),
            new IndexBuffer(device->getDevice(), quadIndicesXPos, 6)
        });

        quads.push_back({
            new VertexBuffer(device->getDevice(), sizeof(quadVerticesXNeg),
                                                     sizeof(float) * 3, quadVerticesXNeg),
            new IndexBuffer(device->getDevice(), quadIndicesXNeg, 6)
        });

        quads.push_back({
            new VertexBuffer(device->getDevice(), sizeof(quadVerticesYPos),
                                                     sizeof(float) * 3, quadVerticesYPos),
            new IndexBuffer(device->getDevice(), quadIndicesYPos, 6),

        });

        quads.push_back({
            new VertexBuffer(device->getDevice(), sizeof(quadVerticesYNeg),
                                                     sizeof(float) * 3, quadVerticesYNeg),
            new IndexBuffer(device->getDevice(), quadIndicesYNeg, 6)
        });

        quads.push_back({
            new VertexBuffer(device->getDevice(), sizeof(quadVerticesZPos),
                                                     sizeof(float) * 3, quadVerticesZPos),
            new IndexBuffer(device->getDevice(), quadIndicesZPos, 6),
        });

        quads.push_back({
            new VertexBuffer(device->getDevice(), sizeof(quadVerticesZNeg),
                                                     sizeof(float) * 3, quadVerticesZNeg),
            new IndexBuffer(device->getDevice(), quadIndicesZNeg, 6)
        });
    }
public:
    void destroy()
    {
        for (auto value : quads)
        {
            delete value.quadMeshIndex;
            delete value.quadMeshVertex;
        }
        sampler->Release();
        delete viewProjMatrixBuff;
        delete irradianceGenerator;
        delete cubemapConvertShader;
    }

  

private:
    static inline float quadVerticesXPos[]
    {
        0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, 0.5f
    };
    static inline uint32_t quadIndicesXPos[]
    {
        3, 2, 1, 0, 3, 1
    };

    static inline float quadVerticesXNeg[]{
        -0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, 0.5f
    };
    static inline uint32_t quadIndicesXNeg[]{
        0, 1, 2, 3, 0, 2
    };
    static inline float quadVerticesYPos[]{
        -0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, 0.5f
    };
    static inline uint32_t quadIndicesYPos[]{
        3, 0, 2, 1, 3, 2
    };
    static inline float quadVerticesYNeg[]
    {
        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, 0.5f
    };
    static inline uint32_t quadIndicesYNeg[]
    {
        3, 1, 2, 0, 3, 2
    };
    static inline float quadVerticesZPos[]
    {
        -0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f
    };
    static inline uint32_t quadIndicesZPos[]
    {
        0, 1, 2, 3, 0, 2
    };
    static inline float quadVerticesZNeg[]
    {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,
        0.5f, 0.5f, -0.5f
    };
    static inline uint32_t quadIndicesZNeg[]{
        1, 3, 2, 0, 1, 2
    };
};
