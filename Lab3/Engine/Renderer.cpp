#include "Renderer.h"

#include <iostream>
#include <random>

#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_dx11.h"
#include "../ImGUI/imgui_impl_win32.h"
#include <DDSTextureLoader.h>

#include "../Utils/FileSystemUtils.h"

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
    D3D11_RASTERIZER_DESC cmdesc = {};
    ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
    cmdesc.FillMode = D3D11_FILL_SOLID;
    cmdesc.CullMode = D3D11_CULL_NONE;
    cmdesc.FrontCounterClockwise = true;
    uint32_t stencRef = 0;
    device.getDeviceContext()->RSGetState(&defaultRasterState);
    device.getDeviceContext()->OMGetDepthStencilState(&defaultDepthState, &stencRef);

    if (FAILED(device.getDevice()->CreateRasterizerState(&cmdesc, &skyboxRasterState)))
    {
        throw std::runtime_error("Failed to create raster state");
    }

    D3D11_DEPTH_STENCIL_DESC dssDesc;
    ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    dssDesc.DepthEnable = true;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    if (FAILED(device.getDevice()->CreateDepthStencilState(&dssDesc, &skyboxDepthState)))
    {
        throw std::runtime_error("Failed to create depth state");
    }
    loadImgui();
    loadCubeMap();
}

void calcSkyboxSize(SkyboxConfig& config, uint32_t width, uint32_t height, float fovDeg)
{
    float n = 0.01f;
    float fov = XMConvertToRadians(fovDeg);
    float halfW = tanf(fov / 2) * n;
    float halfH = height / float(width) * halfW;
    config.size.x = sqrtf(n * n + halfH * halfH + halfW * halfW) * 1.1f;
}

void Renderer::drawFrame()
{
    drawGui();
    shaderConstant.cameraMatrix = camera.getViewMatrix();

    XMMATRIX mProjection = DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90),
                                                             (float)engineWindow->getWidth() / (float)engineWindow->
                                                             getHeight(), 0.001f, 2000.0f);
    shaderConstant.cameraMatrix = XMMatrixMultiply(shaderConstant.cameraMatrix, mProjection);
    skyboxConfig.cameraMatrix = shaderConstant.cameraMatrix;
    skyboxConfig.cameraPosition = camera.getPosition();
    calcSkyboxSize(skyboxConfig, engineWindow->getWidth(), engineWindow->getHeight(), 90);

    lightConstantData.cameraPosition = camera.getPosition();
    constantBuffer->updateData(device.getDeviceContext(), &shaderConstant);
    lightConstant->updateData(device.getDeviceContext(), &lightConstantData);
    pbrConfiguration->updateData(device.getDeviceContext(), &configuration);
    skyboxConfigConstant->updateData(device.getDeviceContext(), &skyboxConfig);
    toneMapper->clearRenderTarget(device.getDeviceContext(), swapChain->getCurrentImage());

    toneMapper->getRendertargetView()->bind(device.getDeviceContext(), engineWindow->getWidth(),
                                            engineWindow->getHeight(), swapChain->getCurrentImage());
    cubeMapShader->bind(device.getDeviceContext());
    device.getDeviceContext()->PSSetSamplers(0, 1, &sampler);
    skyboxConfigConstant->bindToVertexShader(device.getDeviceContext());
    device.getDeviceContext()->PSSetShaderResources(0, 1, &cubeMapTextureResourceView);
    device.getDeviceContext()->OMSetDepthStencilState(skyboxDepthState, 1);
    device.getDeviceContext()->RSSetState(skyboxRasterState);
    cubeMapShader->draw(device.getDeviceContext(), sphereIndex, sphereVertex);

    toneMapper->getRendertargetView()->clearDepthAttachments(device.getDeviceContext());

    shader->bind(device.getDeviceContext());
    device.getDeviceContext()->PSSetSamplers(0, 1, &sampler);
    device.getDeviceContext()->PSSetShaderResources(0, 1, &cubeMapTextureResourceView);

    constantBuffer->bindToVertexShader(device.getDeviceContext());
    lightConstant->bindToPixelShader(device.getDeviceContext());
    pbrConfiguration->bindToPixelShader(device.getDeviceContext(), 1);
    device.getDeviceContext()->OMSetDepthStencilState(defaultDepthState, 1);
    device.getDeviceContext()->RSSetState(defaultRasterState);
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
    vertexInputs.push_back({"COLOR", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32A32_FLOAT});

    shader->makeInputLayout(vertexInputs.data(), (uint32_t)vertexInputs.size());


    shadersInfos.clear();
    shadersInfos.push_back({L"Shaders/skyboxVS.hlsl", VERTEX_SHADER, "Lab3 skybox vertex shader"});
    shadersInfos.push_back({L"Shaders/skyboxPS.hlsl", PIXEL_SHADER, "Lab3 skybox pixel shader"});
    cubeMapShader = Shader::loadShader(device.getDevice(), shadersInfos.data(), shadersInfos.size());;
    cubeMapShader->makeInputLayout(vertexInputs.data(), vertexInputs.size());
}

void Renderer::loadSphere()
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    float color[] = {1.0, 0.8431372, 0, 1.0f};
    makeSphere2(vertices, indices, 25, 52, 52, color, false);
    sphereVertex = shader->createVertexBuffer(device.getDevice(), vertices.size() * sizeof(Vertex), sizeof(Vertex),
                                              vertices.data(),
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
    toneMapper->destroy();
    delete toneMapper;
    sampler->Release();
    skyboxDepthState->Release();
    skyboxRasterState->Release();
    cubeMapTextureResourceView->Release();
    cubeMapTexture->Release();
    delete cubeMapShader;
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

void Renderer::makeSphere(std::vector<float>& verticesOutput, std::vector<uint32_t>& indicesOutput, float radius,
                          uint32_t layerTile, uint32_t circumferenceTile, float* defaultColor,
                          bool generateColors)
{
    uint32_t circCnt = (int)(circumferenceTile + 0.5f);
    if (circCnt < 4) circCnt = 4;
    uint32_t circCnt_2 = circCnt / 2;
    uint32_t layerCount = (int)(layerTile + 0.5f);
    if (layerCount < 2) layerCount = 2;


    std::default_random_engine gen;
    std::uniform_real_distribution<double> colorGenerator(0.0,
                                                          1.0);
    for (uint32_t tbInx = 0; tbInx <= layerCount; tbInx++)
    {
        float v = (1.0 - (float)tbInx / layerCount);
        float heightFac = XMScalarSin((1.0 - 2.0 * tbInx / layerCount) * PI / 2.0);
        float cosUp = sqrt(1.0 - heightFac * heightFac);
        float z = heightFac;
        for (int i = 0; i <= circCnt_2; i++)
        {
            float u = (float)i / (float)circCnt_2;
            float angle = PI * u;
            float x = XMScalarCos(angle) * cosUp;
            float y = XMScalarSin(angle) * cosUp;
            verticesOutput.push_back(x * radius);
            verticesOutput.push_back(y * radius);
            verticesOutput.push_back(z * radius);

            verticesOutput.push_back(x);
            verticesOutput.push_back(y);
            verticesOutput.push_back(z);

            verticesOutput.push_back(u);
            verticesOutput.push_back(v);
            if (generateColors)
            {
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
            }
            if (!generateColors && defaultColor)
            {
                verticesOutput.push_back(defaultColor[0]);
                verticesOutput.push_back(defaultColor[1]);
                verticesOutput.push_back(defaultColor[2]);
            }
        }
        for (int i = 0; i <= circCnt_2; i++)
        {
            float u = (float)i / (float)circCnt_2;
            float angle = PI * u + PI;
            float x = XMScalarCos(angle) * cosUp;
            float y = XMScalarSin(angle) * cosUp;
            verticesOutput.push_back(x * radius);
            verticesOutput.push_back(y * radius);
            verticesOutput.push_back(z * radius);

            verticesOutput.push_back(x);
            verticesOutput.push_back(y);
            verticesOutput.push_back(z);

            verticesOutput.push_back(u);
            verticesOutput.push_back(v);
            if (generateColors)
            {
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
                verticesOutput.push_back(colorGenerator(gen));
            }
            if (!generateColors && defaultColor)
            {
                verticesOutput.push_back(defaultColor[0]);
                verticesOutput.push_back(defaultColor[1]);
                verticesOutput.push_back(defaultColor[2]);
            }
        }
    }

    uint32_t circSize_2 = circCnt_2 + 1;
    uint32_t circSize = circSize_2 * 2;
    for (uint32_t i = 0; i < circCnt_2; i++)
    {
        indicesOutput.push_back(circSize + i);
        indicesOutput.push_back(circSize + i + 1);
        indicesOutput.push_back(i);
    }
    for (uint32_t i = circCnt_2 + 1; i < 2 * circCnt_2 + 1; i++)
    {
        indicesOutput.push_back(circSize + i);
        indicesOutput.push_back(circSize + i + 1);
        indicesOutput.push_back(i);
    }

    for (uint32_t tbInx = 1; tbInx < layerCount - 1; tbInx++)
    {
        uint32_t ringStart = tbInx * circSize;
        uint32_t nextRingStart = (tbInx + 1) * circSize;
        for (int i = 0; i < circCnt_2; i++)
        {
            indicesOutput.push_back(ringStart + i);
            indicesOutput.push_back(nextRingStart + i);
            indicesOutput.push_back(nextRingStart + i + 1);

            indicesOutput.push_back(ringStart + i);
            indicesOutput.push_back(nextRingStart + i + 1);
            indicesOutput.push_back(ringStart + i + 1);
        }
        ringStart += circSize_2;
        nextRingStart += circSize_2;
        for (uint32_t i = 0; i < circCnt_2; i++)
        {
            indicesOutput.push_back(ringStart + i);
            indicesOutput.push_back(nextRingStart + i);
            indicesOutput.push_back(nextRingStart + i + 1);

            indicesOutput.push_back(ringStart + i);
            indicesOutput.push_back(nextRingStart + i + 1);
            indicesOutput.push_back(ringStart + i + 1);
        }
    }
    int start = (layerCount - 1) * circSize;
    for (int i = 0; i < circCnt_2; i++)
    {
        indicesOutput.push_back(start + i + 1);
        indicesOutput.push_back(start + i);
        indicesOutput.push_back(start + i + circSize);
    }
    for (int i = circCnt_2 + 1; i < 2 * circCnt_2 + 1; i++)
    {
        indicesOutput.push_back(start + i + 1);
        indicesOutput.push_back(start + i);
        indicesOutput.push_back(start + i + circSize);
    }
}

void Renderer::makeSphere2(std::vector<Vertex>& verticesOutput, std::vector<uint32_t>& indicesOutput, double radius,
                           double latitudeBands, double longitudeBands, float* defaultColor, bool generateColors)
{
    std::default_random_engine gen;
    std::uniform_real_distribution<double> colorGenerator(0.0,
                                                          1.0);
    uint32_t numVertices = ((latitudeBands - 2) * longitudeBands) + 2;
    uint32_t numIndices = (((latitudeBands - 3) * (longitudeBands) * 2) + (longitudeBands * 2)) * 3;

    float phi = 0.0f;
    float theta = 0.0f;

    verticesOutput.resize(numVertices);

    XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    verticesOutput[0].position[0] = 0.0f;
    verticesOutput[0].position[1] = 0.0f;
    verticesOutput[0].position[2] = 1.0f;
    verticesOutput[0].normal[0] = 0.0f;
    verticesOutput[0].normal[1] = 0.0f;
    verticesOutput[0].normal[2] = 1.0f;

    if (generateColors)
    {
        verticesOutput[0].color[0] = colorGenerator(gen);
        verticesOutput[0].color[1] = colorGenerator(gen);
        verticesOutput[0].color[2] = colorGenerator(gen);

    }
    if (!generateColors && defaultColor)
    {
        verticesOutput[0].color[0] = defaultColor[0];
        verticesOutput[0].color[1] = defaultColor[1];
        verticesOutput[0].color[2] = defaultColor[2];
    }

    for (uint32_t i = 0; i < latitudeBands - 2; i++)
    {
        theta = (i + 1) * (XM_PI / (latitudeBands - 1));
        XMMATRIX Rotationx = XMMatrixRotationX(theta);
        for (uint32_t j = 0; j < longitudeBands; j++)
        {
            phi = j * (XM_2PI / longitudeBands);
            XMMATRIX Rotationy = XMMatrixRotationZ(phi);
            currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy));
            currVertPos = XMVector3Normalize(currVertPos);


            verticesOutput[i * (__int64)longitudeBands + j + 1].position[0] = XMVectorGetX(currVertPos);
            verticesOutput[i * (__int64)longitudeBands + j + 1].position[1] = XMVectorGetY(currVertPos);
            verticesOutput[i * (__int64)longitudeBands + j + 1].position[2] = XMVectorGetZ(currVertPos);
            verticesOutput[i * (__int64)longitudeBands + j + 1].normal[0] = XMVectorGetX(currVertPos);
            verticesOutput[i * (__int64)longitudeBands + j + 1].normal[1] = XMVectorGetY(currVertPos);
            verticesOutput[i * (__int64)longitudeBands + j + 1].normal[2] = XMVectorGetZ(currVertPos);

            if (generateColors)
            {
                verticesOutput[i * (__int64)longitudeBands + j + 1].color[0] = colorGenerator(gen);
                verticesOutput[i * (__int64)longitudeBands + j + 1].color[1] = colorGenerator(gen);
                verticesOutput[i * (__int64)longitudeBands + j + 1].color[2] = colorGenerator(gen);

            }
            if (!generateColors && defaultColor)
            {
                verticesOutput[i * (__int64)longitudeBands + j + 1].color[0] = defaultColor[0];
                verticesOutput[i * (__int64)longitudeBands + j + 1].color[1] = defaultColor[1];
                verticesOutput[i * (__int64)longitudeBands + j + 1].color[2] = defaultColor[2];
            }
        }
    }


    verticesOutput[(__int64)numVertices - 1].position[0] = 0.0f;
    verticesOutput[(__int64)numVertices - 1].position[1] = 0.0f;
    verticesOutput[(__int64)numVertices - 1].position[2] = -1.0f;
    verticesOutput[(__int64)numVertices - 1].normal[0] = 0.0f;
    verticesOutput[(__int64)numVertices - 1].normal[1] = 0.0f;
    verticesOutput[(__int64)numVertices - 1].normal[2] = -1.0f;

    if (generateColors)
    {
        verticesOutput[(__int64)numVertices - 1].color[0] = colorGenerator(gen);
        verticesOutput[(__int64)numVertices - 1].color[1] = colorGenerator(gen);
        verticesOutput[(__int64)numVertices - 1].color[2] = colorGenerator(gen);

    }
    if (!generateColors && defaultColor)
    {
        verticesOutput[(__int64)numVertices - 1].color[0] = defaultColor[0];
        verticesOutput[(__int64)numVertices - 1].color[1] = defaultColor[1];
        verticesOutput[(__int64)numVertices - 1].color[2] = defaultColor[2];
    }
    indicesOutput.resize(numIndices);
    uint32_t k = 0;
    for (uint32_t i = 0; i < longitudeBands - 1; i++)
    {
        indicesOutput[k] = i + 1;
        indicesOutput[(__int64)k + 2] = 0;
        indicesOutput[(__int64)k + 1] = i + 2;
        k += 3;
    }


    indicesOutput[k] = longitudeBands;
    indicesOutput[(__int64)k + 2] = 0;
    indicesOutput[(__int64)k + 1] = 1;
    k += 3;

    for (uint32_t i = 0; i < latitudeBands - 3; i++)
    {
        for (uint32_t j = 0; j < longitudeBands - 1; j++)
        {
            indicesOutput[(__int64)k + 2] = i * longitudeBands + j + 1;
            indicesOutput[(__int64)k + 1] = i * longitudeBands + j + 2;
            indicesOutput[k] = (i + 1) * longitudeBands + j + 1;

            indicesOutput[(__int64)k + 5] = (i + 1) * longitudeBands + j + 1;
            indicesOutput[(__int64)k + 4] = i * longitudeBands + j + 2;
            indicesOutput[(__int64)k + 3] = (i + 1) * longitudeBands + j + 2;

            k += 6;
        }


        indicesOutput[(__int64)k + 2] = (i * longitudeBands) + longitudeBands;
        indicesOutput[(__int64)k + 1] = (i * longitudeBands) + 1;
        indicesOutput[k] = ((i + 1) * longitudeBands) + longitudeBands;

        indicesOutput[(__int64)k + 5] = ((i + 1) * longitudeBands) + longitudeBands;
        indicesOutput[(__int64)k + 4] = (i * longitudeBands) + 1;
        indicesOutput[(__int64)k + 3] = ((i + 1) * longitudeBands) + 1;

        k += 6;
    }

    for (uint32_t i = 0; i < longitudeBands - 1; i++)
    {
        indicesOutput[(__int64)k + 2] = numVertices - 1;
        indicesOutput[k] = (numVertices - 1) - (i + 1);
        indicesOutput[(__int64)k + 1] = (numVertices - 1) - (i + 2);
        k += 3;
    }


    indicesOutput[(__int64)k + 2] = numVertices - 1;
    indicesOutput[k] = (numVertices - 1) - longitudeBands;
    indicesOutput[(__int64)k + 1] = numVertices - 2;
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
    if (ImGui::Combo("Mode", &currentItem, "default\0normal distribution\0geometry function\0fresnel function"))
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
    for (uint32_t i = 0; i < 3; i++)
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
    for (uint32_t i = 0; i < 3; i++)
    {
        lightConstantData.sources[i].position.x = lightsPosition[i][0];
        lightConstantData.sources[i].position.y = lightsPosition[i][1];
        lightConstantData.sources[i].position.z = lightsPosition[i][2];
    }
    ImGui::End();
}


void Renderer::loadConstants()
{
    ZeroMemory(&shaderConstant, sizeof(ShaderConstant));
    shaderConstant.worldMatrix = DirectX::XMMatrixIdentity() * XMMatrixScaling(5, 5, 5);
    constantBuffer = new ConstantBuffer(device.getDevice(), &shaderConstant, sizeof(ShaderConstant),
                                        "Camera and mesh transform matrices");

    lightConstantData.sources[0].position = XMFLOAT3(0, 40, 0);
    lightConstantData.sources[1].position = XMFLOAT3(40, 0, 0);
    lightConstantData.sources[2].position = XMFLOAT3(0, -40, -40);

    lightConstant = new ConstantBuffer(device.getDevice(), &lightConstantData, sizeof(lightConstantData),
                                       "Light sources infos");

    pbrConfiguration = new ConstantBuffer(device.getDevice(), &pbrConfiguration, sizeof(PBRConfiguration),
                                          "PBR configuration buffer");
    skyboxConfig.worldMatrix = DirectX::XMMatrixIdentity();
    skyboxConfigConstant = new ConstantBuffer(device.getDevice(), &skyboxConfig, sizeof(SkyboxConfig),
                                              "Skybox configuration");
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


void Renderer::loadCubeMap()
{
    auto workDir = FileSystemUtils::getCurrentDirectoryPath();
    workDir += L"BrightSky.dds";
    HRESULT res = CreateDDSTextureFromFile(device.getDevice(), device.getDeviceContext(), workDir.c_str(),
                                           &cubeMapTexture, &cubeMapTextureResourceView);
    if (FAILED(res))
    {
        throw std::runtime_error("Failed to load texture from file");
    }
}
