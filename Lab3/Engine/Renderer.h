#pragma once
#include "ToneMapper.h"
#include "../DXDevice/DXSwapChain.h"
#include "../DXDevice/DXDevice.h"
#include "../DXShader/Shader.h"
#include "../DXShader/ConstantBuffer.h"
#include "Camera/Camera.h"
#include <d3d11_1.h>

struct PBRConfiguration
{
    int defaultFunction = 1;
    int normalDistribution = 0;
    int fresnelFunction = 0;
    int geometryFunction = 0;

    float metallic = 1.0;
    float roughness = 0.0;
    float ambientIntensity = 0.3f;
    float alignment;
};


struct ShaderConstant
{
    XMMATRIX worldMatrix;
    XMMATRIX cameraMatrix;
};

struct PointLightSource
{
    XMFLOAT3 position;
    float intensity = 1;
};

struct LightConstant
{
    PointLightSource sources[3];
    XMFLOAT3 cameraPosition;
};

class Renderer : public IWindowKeyCallback
{
private:
    static DXSwapChain* swapChain;
    static void resizeCallback(uint32_t width, uint32_t height);
    static Renderer* instance;

public:
    Renderer(Window* window);

private:
    Window* engineWindow;
    DXDevice device;
    std::vector<WindowKey> keys;
    Shader* shader;
    ShaderConstant shaderConstant{};
    alignas(256) LightConstant lightConstantData{};
    PBRConfiguration configuration;
    ConstantBuffer* constantBuffer;
    ConstantBuffer* lightConstant;
    ConstantBuffer* pbrConfiguration;
    ToneMapper* toneMapper;
    ID3D11SamplerState* sampler;
    VertexBuffer* sphereVertex = nullptr;
    IndexBuffer* sphereIndex = nullptr;
    Camera camera;
    ID3DUserDefinedAnnotation* annotation;

public:
    void drawFrame();
    void release();
    void keyEvent(WindowKey key) override;
    WindowKey* getKeys(uint32_t* pKeysAmountOut) override;
    void makeSphere(std::vector<float>& verticesOutput, std::vector<uint32_t>& indicesOutput, float radius, uint32_t layerTile, uint32_t circumferenceTile);
private:
    void drawGui();
    void loadShader();
    void loadSphere();
    void loadConstants();
    void loadImgui();
};
