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
    void initialize(ID3D11Device* device, uint32_t width, uint32_t height);

    
    void resize(uint32_t width, uint32_t height);
    void makeBrightnessMaps(ID3D11DeviceContext* deviceContext);
    void postProcessToneMap(ID3D11DeviceContext* deviceContext);

    DXRenderTargetView* getRendertargetView();

    void clearRenderTarget(ID3D11DeviceContext* deviceContext);

private:
    void createTextures(ID3D11Device* device, uint32_t width, uint32_t height);
    void createSquareTexture(ID3D11Device* device, Texture& text, uint32_t len);
    void destroyScaledBrighnessMaps();
};
