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

    float metallic = 0.9;
    float roughness = 0.03;
    float ambientIntensity = 3.0f;
    float alignment;
};


struct ShaderConstant
{
    XMMATRIX worldMatrix;
    XMMATRIX cameraMatrix;
};

struct SkyboxConfig
{
    XMMATRIX worldMatrix;
    XMMATRIX cameraMatrix;
    XMFLOAT4 size;
    XMFLOAT3 cameraPosition;

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

struct Vertex
{
    float position[3];
    float normal[3];
    float uv[2];
    float color[3];
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
    Shader* cubeMapShader;
    ShaderConstant shaderConstant{};
    alignas(256) LightConstant lightConstantData{};
    PBRConfiguration configuration;
    SkyboxConfig skyboxConfig{};
    ConstantBuffer* constantBuffer;
    ConstantBuffer* lightConstant;
    ConstantBuffer* pbrConfiguration;
    ConstantBuffer* skyboxConfigConstant;
    ToneMapper* toneMapper;
    ID3D11SamplerState* sampler;
    VertexBuffer* sphereVertex = nullptr;
    IndexBuffer* sphereIndex = nullptr;
    Camera camera;
    ID3DUserDefinedAnnotation* annotation;
    ID3D11Resource* cubeMapTexture;
    ID3D11ShaderResourceView* cubeMapTextureResourceView;

    ID3D11DepthStencilState* skyboxDepthState;
    ID3D11RasterizerState* skyboxRasterState;

    ID3D11DepthStencilState* defaultDepthState;
    ID3D11RasterizerState* defaultRasterState;
public:
    void drawFrame();
    void release();
    void keyEvent(WindowKey key) override;
    WindowKey* getKeys(uint32_t* pKeysAmountOut) override;
    void makeSphere(std::vector<float>& verticesOutput, std::vector<uint32_t>& indicesOutput, float radius,
                    uint32_t layerTile, uint32_t circumferenceTile,float* defaultColor, bool generateColors);
    void makeSphere2(std::vector<Vertex>& verticesOutput, std::vector<uint32_t>& indicesOutput, double  radius,
                    double  latitudeBands , double longitudeBands ,float* defaultColor, bool generateColors);
    void makesphere3(std::vector<float>& verticesOutput, std::vector<uint32_t>& indicesOutput, float* defaultColor);
private:
    void drawGui();
    void loadShader();
    void loadSphere();
    void loadConstants();
    void loadImgui();
    void loadCubeMap();
};
