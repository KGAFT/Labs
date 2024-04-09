#include "Renderer.h"

#include <iostream>
#include <random>

#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_dx11.h"
#include "../ImGUI/imgui_impl_win32.h"
#include <ktx.h>

#define PI 3.14159265359

DXSwapChain* Renderer::swapChain = nullptr;
Renderer* Renderer::instance = nullptr;

void Renderer::resizeCallback(uint32_t width, uint32_t height)
{
    if (swapChain)
    {
        swapChain->resize(width, height);
        instance->toneMapper->resize(width, height);
    }
}

Renderer::Renderer(Window* window) : engineWindow(window)
{
    instance = this;
    swapChain = device.getSwapChain(window, "Lab3 default swap chain");
    window->addResizeCallback(resizeCallback);
    window->getInputSystem()->addKeyCallback(&camera);
    window->getInputSystem()->addMouseCallback(&camera);
    window->getInputSystem()->addKeyCallback(this);
    loadShader();
    loadSphere();
    loadConstants();
    window->getInputSystem()->addKeyCallback(this);
    keys.push_back({DIK_F1, KEY_DOWN});
    keys.push_back({DIK_F2, KEY_DOWN});
    keys.push_back({DIK_F3, KEY_DOWN});
    device.getDeviceContext()->QueryInterface(IID_PPV_ARGS(&annotation));
    toneMapper = new ToneMapper(device.getDevice(), annotation);
    toneMapper->initialize(window->getWidth(), window->getHeight(), DX_SWAPCHAIN_DEFAULT_BUFFER_AMOUNT);

    D3D11_SAMPLER_DESC desc = {};

    desc.Filter = D3D11_FILTER_ANISOTROPIC;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MinLOD = -D3D11_FLOAT32_MAX;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;

    if (FAILED(device.getDevice()->CreateSamplerState(&desc, &sampler)))
    {
        throw std::runtime_error("Failed to create sampler");
    }
    loadImgui();
    loadCubeMap();
}

void Renderer::drawFrame()
{
    drawGui();
    shaderConstant.cameraMatrix = camera.getViewMatrix();

    XMMATRIX mProjection = DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90),
                                                             (float)engineWindow->getWidth() / (float)engineWindow->
                                                             getHeight(), 0.001f, 2000.0f);
    shaderConstant.cameraMatrix = XMMatrixMultiply(shaderConstant.cameraMatrix, mProjection);
    
    lightConstantData.cameraPosition = camera.getPosition();
    constantBuffer->updateData(device.getDeviceContext(), &shaderConstant);
    lightConstant->updateData(device.getDeviceContext(), &lightConstantData);
    pbrConfiguration->updateData(device.getDeviceContext(), &configuration);
    toneMapper->clearRenderTarget(device.getDeviceContext(), swapChain->getCurrentImage());

    shader->bind(device.getDeviceContext());
    toneMapper->getRendertargetView()->bind(device.getDeviceContext(), engineWindow->getWidth(),
                                            engineWindow->getHeight(), swapChain->getCurrentImage());
    constantBuffer->bindToVertexShader(device.getDeviceContext());
    lightConstant->bindToPixelShader(device.getDeviceContext());
    pbrConfiguration->bindToPixelShader(device.getDeviceContext(), 1);
    shader->draw(device.getDeviceContext(), sphereIndex, sphereVertex);

    toneMapper->makeBrightnessMaps(device.getDeviceContext(), swapChain->getCurrentImage());
    swapChain->clearRenderTargets(device.getDeviceContext(), 0, 0, 0, 1.0f);
    device.getDeviceContext()->PSSetSamplers(0, 1, &sampler);

    swapChain->bind(device.getDeviceContext(), engineWindow->getWidth(), engineWindow->getHeight());
    toneMapper->postProcessToneMap(device.getDeviceContext(), swapChain->getCurrentImage());
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    toneMapper->unBindRenderTargets(device.getDeviceContext());
    swapChain->present(true);
}

void Renderer::loadShader()
{
    std::vector<ShaderCreateInfo> shadersInfos;
    shadersInfos.push_back({L"Shaders/VertexShader.hlsl", VERTEX_SHADER, "Lab3 cube vertex shader"});
    shadersInfos.push_back({L"Shaders/PBRPixelShader.hlsl", PIXEL_SHADER, "Lab3 cube pixel shader"});
    shader = Shader::loadShader(device.getDevice(), shadersInfos.data(), (uint32_t)shadersInfos.size());
    std::vector<ShaderVertexInput> vertexInputs;
    vertexInputs.push_back({"POSITION", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT});
    vertexInputs.push_back({"UV", 0, sizeof(float) * 2, DXGI_FORMAT_R32G32_FLOAT});
    vertexInputs.push_back({"NORMAL", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT});
    vertexInputs.push_back({"COLOR", 0, sizeof(float) * 4, DXGI_FORMAT_R32G32B32A32_FLOAT});

    shader->makeInputLayout(vertexInputs.data(), (uint32_t)vertexInputs.size());
}

void Renderer::loadSphere()
{

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    makeSphere(vertices, indices, 25, 52, 52, true);
    sphereVertex = shader->createVertexBuffer(device.getDevice(), vertices.size()*sizeof(float), 12 * sizeof(float), vertices.data(),
                                            "Sphere vertex buffer");
    sphereIndex = shader->createIndexBuffer(device.getDevice(), indices.data(), indices.size(),
                                          "Sphere index buffer");
}

void Renderer::release()
{
    delete sphereVertex;
    delete sphereIndex;
    delete constantBuffer;
    delete shader;
    delete swapChain;
}

void Renderer::keyEvent(WindowKey key)
{
    uint32_t index = key.key - DIK_F1;
    lightConstantData.sources[index].intensity *= 100;
    if (lightConstantData.sources[index].intensity > 1000000)
    {
        lightConstantData.sources[index].intensity = 1;
    }
}

WindowKey* Renderer::getKeys(uint32_t* pKeysAmountOut)
{
    *pKeysAmountOut = (uint32_t)keys.size();
    return keys.data();
}

void Renderer::makeSphere(std::vector<float>& verticesOutput, std::vector<uint32_t>& indicesOutput, float radius, uint32_t layerTile, uint32_t circumferenceTile,
                          bool generateColors)
{
    
    uint32_t circCnt = (int)( circumferenceTile + 0.5f );
    if ( circCnt < 4 ) circCnt = 4;
    uint32_t circCnt_2 = circCnt / 2;
    uint32_t layerCount = (int)( layerTile + 0.5f );
    if ( layerCount < 2 ) layerCount = 2;


    std::default_random_engine gen;
    std::uniform_real_distribution<double> colorGenerator(0.0,
                                                        1.0);
    for ( uint32_t tbInx = 0; tbInx <= layerCount; tbInx ++ )
    {
        float v = ( 1.0 - (float)tbInx / layerCount );
        float heightFac = XMScalarSin( (1.0 - 2.0 * tbInx / layerCount ) * PI/2.0 );
        float cosUp = sqrt( 1.0 - heightFac * heightFac );
        float z = heightFac;
        for ( int i = 0; i <= circCnt_2; i ++ )
        {
            float u = (float)i / (float)circCnt_2;
            float angle = PI * u;
            float x = XMScalarCos( angle ) * cosUp;
            float y = XMScalarSin( angle ) * cosUp;
            verticesOutput.push_back( x * radius);
            verticesOutput.push_back(y * radius);
            verticesOutput.push_back(z * radius);

            verticesOutput.push_back(x);
            verticesOutput.push_back(y);
            verticesOutput.push_back(z);

            verticesOutput.push_back(u);
            verticesOutput.push_back(v);
            if(generateColors)
            {
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
            }
           
        }
        for ( int i = 0; i <= circCnt_2; i ++ )
        {
            float u = (float)i / (float)circCnt_2;
            float angle = PI * u + PI;
            float x = XMScalarCos( angle ) * cosUp;
            float y = XMScalarSin( angle ) * cosUp;
            verticesOutput.push_back( x * radius);
            verticesOutput.push_back(y * radius);
            verticesOutput.push_back(z * radius);

            verticesOutput.push_back(x);
            verticesOutput.push_back(y);
            verticesOutput.push_back(z);

            verticesOutput.push_back(u);
            verticesOutput.push_back(v);
            if(generateColors)
            {
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
            }
        }
    }

    uint32_t circSize_2 = circCnt_2 + 1;
    uint32_t circSize = circSize_2 * 2;
    for ( uint32_t i = 0; i < circCnt_2; i ++  )
    {
        indicesOutput.push_back(circSize + i);
        indicesOutput.push_back(circSize + i + 1);
        indicesOutput.push_back(i);
    }
    for ( uint32_t i = circCnt_2+1; i < 2*circCnt_2+1; i ++ ){
        indicesOutput.push_back(circSize + i);
        indicesOutput.push_back(circSize + i + 1);
        indicesOutput.push_back(i);
    }

    for ( uint32_t tbInx = 1; tbInx < layerCount - 1; tbInx ++ )
    {
        uint32_t ringStart = tbInx * circSize;
        uint32_t nextRingStart = (tbInx+1) * circSize;
        for ( int i = 0; i < circCnt_2; i ++ )
        {
            indicesOutput.push_back(ringStart + i);
            indicesOutput.push_back(nextRingStart + i);
            indicesOutput.push_back(nextRingStart + i + 1);

            indicesOutput.push_back(ringStart+i);
            indicesOutput.push_back(nextRingStart + i + 1);
            indicesOutput.push_back(ringStart + i + 1 );
        }
        ringStart += circSize_2;
        nextRingStart += circSize_2;
        for ( uint32_t i = 0; i < circCnt_2; i ++ )
        {
            indicesOutput.push_back(ringStart + i);
            indicesOutput.push_back(nextRingStart + i);
            indicesOutput.push_back(nextRingStart + i + 1);

            indicesOutput.push_back(ringStart+i);
            indicesOutput.push_back(nextRingStart + i + 1);
            indicesOutput.push_back(ringStart + i + 1 );
        }
    }
    int start = (layerCount-1) * circSize;
    for ( int i = 0; i < circCnt_2; i ++ )
    {
        indicesOutput.push_back(start+i+1);
        indicesOutput.push_back(start + i);
        indicesOutput.push_back(start + i + circSize);
    }
    for ( int i = circCnt_2+1; i < 2*circCnt_2+1; i ++ )
    {
        indicesOutput.push_back(start+i+1);
        indicesOutput.push_back(start + i);
        indicesOutput.push_back(start + i + circSize);
    }
}

void Renderer::drawGui()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("PBR configuration: ");
    ImGui::Text("Light pbr configuration: ");
    ImGui::SliderFloat("Ambient intensity", &configuration.ambientIntensity, 0, 50);

    static int currentItem = 0;
    if(ImGui::Combo("Mode", &currentItem, "default\0normal distribution\0geometry function\0fresnel function"))
    {
        configuration.fresnelFunction = 0;
        configuration.geometryFunction = 0;
        configuration.normalDistribution = 0;
        configuration.defaultFunction = 0;
        switch (currentItem)
        {
        case 0:
            configuration.defaultFunction = 1;
            break;
        case 1:
            configuration.normalDistribution = 1;
            break;
        case 2:
            configuration.geometryFunction = 1;
            break;
        case 3:
            configuration.fresnelFunction = 1;
            break;
        }
    }
   
    ImGui::Text("Mesh configuration");
    ImGui::SliderFloat("Metallic value", &configuration.metallic, 0, 1);
    ImGui::SliderFloat("Roughness value", &configuration.roughness, 0, 1);
    ImGui::Text("Lights configuration");
    float lightsPosition[3][3];
    for (uint32_t i = 0; i<3;i++)
    {
        lightsPosition[i][0] = lightConstantData.sources[i].position.x;
        lightsPosition[i][1] = lightConstantData.sources[i].position.y;
        lightsPosition[i][2] = lightConstantData.sources[i].position.z;
    }
    ImGui::DragFloat3("Light 1 position", lightsPosition[0]);
    ImGui::SliderFloat("Light 1 intensity", &lightConstantData.sources[0].intensity, 0, 10000);
    ImGui::DragFloat3("Light 2 position", lightsPosition[1]);
    ImGui::SliderFloat("Light 2 intensity", &lightConstantData.sources[1].intensity, 0, 10000);
    ImGui::DragFloat3("Light 3 position", lightsPosition[2]);
    ImGui::SliderFloat("Light 3 intensity", &lightConstantData.sources[2].intensity, 0, 10000);
    ImGui::End();
}


void Renderer::loadConstants()
{
    ZeroMemory(&shaderConstant, sizeof(ShaderConstant));
    shaderConstant.worldMatrix = DirectX::XMMatrixIdentity()*XMMatrixScaling(0.1, 0.1, 0.1);
    constantBuffer = new ConstantBuffer(device.getDevice(), &shaderConstant, sizeof(ShaderConstant),
                                        "Camera and mesh transform matrices");

    lightConstantData.sources[0].position = XMFLOAT3(0, 5, 0);
    lightConstantData.sources[1].position = XMFLOAT3(5, 0, 0);
    lightConstantData.sources[2].position = XMFLOAT3(0, -5, -5);

    lightConstant = new ConstantBuffer(device.getDevice(), &lightConstantData, sizeof(lightConstantData),
                                       "Light sources infos");

    pbrConfiguration = new ConstantBuffer(device.getDevice(), &pbrConfiguration, sizeof(PBRConfiguration),
                                          "PBR configuration buffer");
}

void Renderer::loadImgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    bool result = ImGui_ImplWin32_Init(engineWindow->getWindowHandle());
    if (result)
    {
        result = ImGui_ImplDX11_Init(device.getDevice(), device.getDeviceContext());
    }
    if (!result)
    {
        throw std::runtime_error("Failed to initialize imgui");
    }
}

KTX_error_code textureCallback(int miplevel, int face, int width, int height, int depth, ktx_uint64_t faceLodSize, void *pixels, void *userdata)
{
    std::map<void*, size_t>* target = (std::map<void*, size_t>*) userdata;
    (*target)[pixels] = faceLodSize;
    return KTX_SUCCESS;
}

void Renderer::loadCubeMap()
{
    ktxTexture* texture;
    KTX_error_code res = ktxTexture_CreateFromNamedFile("Images/bluecloud.ktx2",
                                      KTX_TEXTURE_CREATE_NO_FLAGS,
                                      &texture);
    if(res!=KTX_SUCCESS)
    {
        throw std::runtime_error("Failed to load image");
    }
    ktxTexture_IterateLoadLevelFaces(texture, textureCallback, &cubeMapData);
    
}
