#include "Renderer.h"

#include <iostream>
#include <random>

#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_dx11.h"
#include "../ImGUI/imgui_impl_win32.h"

#include "tiny_obj_loader.h"
#include "../STB/stb_image.h"

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
    swapChain = device.getSwapChain(window, "Lab5 default swap chain");
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
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
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

    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    if (FAILED(device.getDevice()->CreateSamplerState(&desc, &avgSampler)))
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

#ifdef _DEBUG
    annotation->BeginEvent(L"Clear render targets");
#endif
    
    toneMapper->clearRenderTarget(device.getDeviceContext(), swapChain->getCurrentImage());
#ifdef _DEBUG
    annotation->EndEvent();
    annotation->EndEvent();
#endif
#ifdef _DEBUG
    annotation->BeginEvent(L"Rendering skybox");
#endif
    toneMapper->getRendertargetView()->bind(device.getDeviceContext(), engineWindow->getWidth(),
                                            engineWindow->getHeight(), swapChain->getCurrentImage());
    cubeMapShader->bind(device.getDeviceContext());
    device.getDeviceContext()->PSSetSamplers(0, 1, &sampler);
    skyboxConfigConstant->bindToVertexShader(device.getDeviceContext());
    device.getDeviceContext()->PSSetShaderResources(0, 1, &cubemap.cubemapSRV);
    device.getDeviceContext()->OMSetDepthStencilState(skyboxDepthState, 1);
    device.getDeviceContext()->RSSetState(skyboxRasterState);
    cubeMapShader->draw(device.getDeviceContext(), sphereIndex, sphereVertex);

    toneMapper->getRendertargetView()->clearDepthAttachments(device.getDeviceContext());
#ifdef _DEBUG
    annotation->EndEvent();
#endif
#ifdef _DEBUG
    annotation->BeginEvent(L"Rendering pbr light");
#endif
    shader->bind(device.getDeviceContext());
    ID3D11SamplerState* samplers[] = {sampler, avgSampler};
    
    device.getDeviceContext()->PSSetSamplers(0, 2, samplers);
    ID3D11ShaderResourceView* resources[] = {cubemap.irradianceSRV, cubemap.prefilteredSRV, cubemap.brdfSRV};
    
    device.getDeviceContext()->PSSetShaderResources(0, 3, resources);

    constantBuffer->bindToVertexShader(device.getDeviceContext());
    lightConstant->bindToPixelShader(device.getDeviceContext());
    pbrConfiguration->bindToPixelShader(device.getDeviceContext(), 1);
    device.getDeviceContext()->OMSetDepthStencilState(defaultDepthState, 1);
    device.getDeviceContext()->RSSetState(defaultRasterState);
    shader->draw(device.getDeviceContext(), sphereIndex, sphereVertex);
#ifdef _DEBUG
    annotation->EndEvent();
#endif
    toneMapper->makeBrightnessMaps(device.getDeviceContext(), swapChain->getCurrentImage());
    swapChain->clearRenderTargets(device.getDeviceContext(), 0, 0, 0, 1.0f);
    device.getDeviceContext()->PSSetSamplers(0, 1, &sampler);

    swapChain->bind(device.getDeviceContext(), engineWindow->getWidth(), engineWindow->getHeight());
    toneMapper->postProcessToneMap(device.getDeviceContext(), swapChain->getCurrentImage());
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    DXDevice::unBindRenderTargets(device.getDeviceContext());
    swapChain->present(true);
}


void Renderer::loadShader()
{
    std::vector<ShaderCreateInfo> shadersInfos;
    shadersInfos.push_back({L"Shaders/Lighting/VertexShader.hlsl", VERTEX_SHADER, "Lab5 cube vertex shader"});
    shadersInfos.push_back({L"Shaders/Lighting/PBRPixelShader.hlsl", PIXEL_SHADER, "Lab5 cube pixel shader"});
    shader = Shader::loadShader(device.getDevice(), shadersInfos.data(), (uint32_t)shadersInfos.size());
    std::vector<ShaderVertexInput> vertexInputs;
    vertexInputs.push_back({"POSITION", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT});
    vertexInputs.push_back({"UV", 0, sizeof(float) * 2, DXGI_FORMAT_R32G32_FLOAT});
    vertexInputs.push_back({"NORMAL", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT});
    vertexInputs.push_back({"COLOR", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32A32_FLOAT});

    shader->makeInputLayout(device.getDevice(), vertexInputs.data(), (uint32_t)vertexInputs.size());


    shadersInfos.clear();
    shadersInfos.push_back({L"Shaders/Skybox/skyboxVS.hlsl", VERTEX_SHADER, "Lab5 skybox vertex shader"});
    shadersInfos.push_back({L"Shaders/Skybox/skyboxPS.hlsl", PIXEL_SHADER, "Lab5 skybox pixel shader"});
    cubeMapShader = Shader::loadShader(device.getDevice(), shadersInfos.data(), shadersInfos.size());;
    cubeMapShader->makeInputLayout(device.getDevice(), vertexInputs.data(), vertexInputs.size());
}

void Renderer::loadSphere()
{
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    float color[] = {0.541, 0, 0.82745};
    makesphere3(vertices, indices, color);
    sphereVertex = new VertexBuffer(device.getDevice(), vertices.size() * sizeof(float), sizeof(float)*11,
                                              vertices.data(),
                                              "Sphere vertex buffer");
    sphereIndex = new IndexBuffer(device.getDevice(), indices.data(), indices.size(),
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
    cubemap.cubemapSRV->Release();
    cubemap.cubemapTexture->Release();
    
    cubemap.irradianceSRV->Release();
    cubemap.irradianceTexture->Release();
    
    cubemap.prefilteredSRV->Release();
    cubemap.prefilteredTexture->Release();
    cubemap.brdfTexture->Release();
    cubemap.brdfSRV->Release();
    
    annotation->Release();

    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui::DestroyContext();
    delete cubeMapShader;
    delete lightConstant;
    delete pbrConfiguration;
    delete skyboxConfigConstant;
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




void Renderer::makesphere3(std::vector<float>& verticesOutput, std::vector<uint32_t>& indicesOutput, float* defaultColor)
{
    tinyobj::attrib_t inattrib;
    std::vector<tinyobj::shape_t> inshapes;
    std::vector<tinyobj::material_t> materials;
    
    auto workDir = FileSystemUtils::getCurrentDirectoryPath();
    workDir+=L"sphere.wvf";
    std::string s( workDir.begin(), workDir.end() );
    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&inattrib, &inshapes, &materials, &err, s.c_str());
    if (!err.empty()) {
        std::cerr << err << std::endl;
        return;
    }
    
    uint32_t idCounter = 0;
    for (auto shape : inshapes)
    {
        for (auto index : shape.mesh.indices)
        {
            verticesOutput.push_back(inattrib.vertices[index.vertex_index*3]);
            verticesOutput.push_back(inattrib.vertices[index.vertex_index*3+1]);
            verticesOutput.push_back(inattrib.vertices[index.vertex_index*3+2]);

            verticesOutput.push_back(inattrib.texcoords[index.texcoord_index*2]);
            verticesOutput.push_back(inattrib.texcoords[index.texcoord_index*2+1]);
            
            verticesOutput.push_back(inattrib.normals[index.normal_index*3]);
            verticesOutput.push_back(inattrib.normals[index.normal_index*3+1]);
            verticesOutput.push_back(inattrib.normals[index.normal_index*3+2]);

            verticesOutput.push_back(defaultColor[0]);
            verticesOutput.push_back(defaultColor[1]);
            verticesOutput.push_back(defaultColor[2]);

            indicesOutput.push_back(idCounter);
            idCounter++;
        }
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
    ImGui::SliderFloat("Metallic value", &configuration.metallic, 0.001, 1);
    ImGui::SliderFloat("Roughness value", &configuration.roughness, 0.001, 1);
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
    shaderConstant.worldMatrix = DirectX::XMMatrixIdentity()*XMMatrixScaling(3, 3, 3);
    constantBuffer = new ConstantBuffer(device.getDevice(), &shaderConstant, sizeof(ShaderConstant),
                                        "Camera and mesh transform matrices");

    lightConstantData.sources[0].position = XMFLOAT3(0, 5, 0);
    lightConstantData.sources[1].position = XMFLOAT3(-5, 0, 0);
    lightConstantData.sources[2].position = XMFLOAT3(0, -5, -5);

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
    CubemapGenerator generator(&device);
    generator.loadHDRCubemap("hdr_room2.hdr", &cubemap);

    cubemap.sourceTexture->Release();
    cubemap.sourceResourceView->Release();
    generator.destroy();
}
