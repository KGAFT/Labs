#include "Renderer.h"

DXSwapChain* Renderer::swapChain = nullptr;

void Renderer::resizeCallback(uint32_t width, uint32_t height) {
    if (swapChain) {
        swapChain->resize(width, height);
    }
}

Renderer::Renderer(Window* window) : engineWindow(window) {
	swapChain = device.getSwapChain(window, "Lab 1 default swap chain");
    window->addResizeCallback(resizeCallback);
    window->getInputSystem()->addKeyCallback(&camera);
    window->getInputSystem()->addMouseCallback(&camera);
    loadShader();
    loadCube();
    loadConstants();
    window->getInputSystem()->addKeyCallback(this);
    keys.push_back({ VK_SPACE, false, KEY_DOWN });
    keys.push_back({ VK_F1, false, KEY_DOWN });
    keys.push_back({ VK_F2, false, KEY_DOWN });
    keys.push_back({ VK_F3, false, KEY_DOWN });
    toneMapper.initialize(device.getDevice(), window->getWidth(), window->getHeight());

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
      
    if(FAILED(device.getDevice()->CreateSamplerState(&desc, &sampler)))
    {
        throw std::runtime_error("Failed to create sampler");
    }
}

void Renderer::drawFrame() {
    shaderConstant.cameraMatrix = camera.getViewMatrix();

    DirectX::XMMATRIX mProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90), (float)engineWindow->getWidth() / (float)engineWindow->getHeight(), 0.001, 2000);
    shaderConstant.cameraMatrix = XMMatrixMultiply(shaderConstant.cameraMatrix, mProjection);
    lightConstantData.cameraPosition = camera.getPosition();
    constantBuffer->updateData(device.getDeviceContext(), &shaderConstant);
    lightConstant->updateData(device.getDeviceContext(), &lightConstantData);
    
    toneMapper.clearRenderTarget(device.getDeviceContext());
    
    toneMapper.getRendertargetView()->bind(device.getDeviceContext(), engineWindow->getWidth(), engineWindow->getHeight(), 0);
    shader->bind(device.getDeviceContext());
    constantBuffer->bindToVertexShader(device.getDeviceContext());
    lightConstant->bindToPixelShader(device.getDeviceContext());
    shader->draw(device.getDeviceContext(), cubeIndex, cubeVertex);

   
    
    toneMapper.renderBrightness(device.getDeviceContext());

    swapChain->clearRenderTargets(device.getDeviceContext(), 0,0,0, 1.0f);
    device.getDeviceContext()->PSSetSamplers(0, 1, &sampler);

    swapChain->bind(device.getDeviceContext(), engineWindow->getWidth(), engineWindow->getHeight());
    toneMapper.renderTonemap(device.getDeviceContext());
    
    


/*
    constantBuffer->updateData(device.getDeviceContext(), &shaderConstant);
    shader->bind(device.getDeviceContext());
    constantBuffer->bindToVertexShader(device.getDeviceContext());
    lightConstant->bindToPixelShader(device.getDeviceContext());
    shader->draw(device.getDeviceContext(), cubeIndex, cubeVertex);
*/
    swapChain->present(true);
}

void Renderer::loadShader() {
    std::vector<ShaderCreateInfo> shadersInfos;
    shadersInfos.push_back({ L"Shaders/VertexShader.hlsl", VERTEX_SHADER, "Lab1 cube vertex shader"});
    shadersInfos.push_back({ L"Shaders/PixelShader.hlsl", PIXEL_SHADER,  "Lab1 cube pixel shader" });
    shader = Shader::loadShader(device.getDevice(), shadersInfos.data(), shadersInfos.size());
    std::vector< ShaderVertexInput> vertexInputs;
    vertexInputs.push_back({ "POSITION", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT });
    vertexInputs.push_back({ "UV", 0, sizeof(float) * 2, DXGI_FORMAT_R32G32_FLOAT });
    vertexInputs.push_back({ "NORMAL", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT });

    shader->makeInputLayout(vertexInputs.data(), vertexInputs.size());
}

void Renderer::loadCube() {
    float vertices[] = {
       -2.0, -2.0,  2.0, 0,1, 0,-1,0,
        2.0, -2.0,  2.0, 1,1, 0,-1,0,
        2.0, -2.0, -2.0, 1,0, 0,-1,0,
       -2.0, -2.0, -2.0, 0,0, 0,-1,0,

       -2.0,  2.0, -2.0, 0,1, 0,1,0,
        2.0,  2.0, -2.0, 1,1, 0,1,0,
        2.0,  2.0,  2.0, 1,0, 0,1,0,
       -2.0,  2.0,  2.0, 0,0, 0,1,0,

        2.0, -2.0, -2.0, 0,1, 1,0,0,
        2.0, -2.0,  2.0, 1,1, 1,0,0,
        2.0,  2.0,  2.0, 1,0, 1,0,0,
        2.0,  2.0, -2.0, 0,0, 1,0,0,

       -2.0, -2.0,  2.0, 0,1, -1,0,0,
       -2.0, -2.0, -2.0, 1,1, -1,0,0,
       -2.0,  2.0, -2.0, 1,0, -1,0,0,
       -2.0,  2.0,  2.0, 0,0, -1,0,0,

        2.0, -2.0,  2.0, 0,1, 0,0,1,
       -2.0, -2.0,  2.0, 1,1, 0,0,1,
       -2.0,  2.0,  2.0, 1,0, 0,0,1,
        2.0,  2.0,  2.0, 0,0, 0,0,1,

       -2.0, -2.0, -2.0, 0,1, 0,0,-1,
        2.0, -2.0, -2.0, 1,1, 0,0,-1,
        2.0,  2.0, -2.0, 1,0, 0,0,-1,
       -2.0,  2.0, -2.0, 0,0, 0,0,-1
    };
    uint32_t indices[] = {
        0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22
    };
    
    cubeVertex = shader->createVertexBuffer(device.getDevice(), sizeof(vertices), 8 * sizeof(float), vertices, "Cube vertex buffer");
    cubeIndex = shader->createIndexBuffer(device.getDevice(), indices, sizeof(indices) / sizeof(uint32_t), "Cube index buffer");
}

void Renderer::release() {
    delete cubeVertex;
    delete cubeIndex;
    delete constantBuffer;
    delete shader;
    delete swapChain;
}

void Renderer::keyEvent(WindowKey key)
{
    uint32_t index = key.key - VK_F1;
    lightConstantData.sources[index].intensity *= 10;
    if (lightConstantData.sources[index].intensity > 100) {
        lightConstantData.sources[index].intensity = 1;
    }
}

WindowKey* Renderer::getKeys(uint32_t* pKeysAmountOut)
{
    *pKeysAmountOut = keys.size();
    return keys.data();
}


void Renderer::loadConstants() {
    ZeroMemory(&shaderConstant, sizeof(ShaderConstant));
    shaderConstant.worldMatrix = DirectX::XMMatrixIdentity();
    constantBuffer = new ConstantBuffer(device.getDevice(), &shaderConstant, sizeof(ShaderConstant), "Camera and mesh transform matrices");

    lightConstantData.sources[0].position = XMFLOAT3(0,5,0);
    lightConstantData.sources[1].position = XMFLOAT3(5,0,0);
    lightConstantData.sources[2].position = XMFLOAT3(0,-5,-5);

    lightConstant = new ConstantBuffer(device.getDevice(), &lightConstantData, sizeof(lightConstantData), "Light sources infos");

}