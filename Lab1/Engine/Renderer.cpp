#include "Renderer.h"

DXSwapChain* Renderer::swapChain = nullptr;

void Renderer::resizeCallback(uint32_t width, uint32_t height) {
    if (swapChain) {
        swapChain->resize(width, height);
    }
}

Renderer::Renderer(Window* window) : engineWindow(window) {
	swapChain = device.getSwapChain(window);
    window->addResizeCallback(resizeCallback);
    loadShader();
    loadCube();
    loadConstants();
}

void Renderer::drawFrame() {
    swapChain->clearRenderTargets(device.getDeviceContext(), 0.1f, 0.2f, 0.5f, 1.0f);
    swapChain->bind(device.getDeviceContext(), engineWindow->getWidth(), engineWindow->getHeight());

    shaderConstant.cameraMatrix = camera.getViewMatrix();

    DirectX::XMMATRIX mProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90), (float)engineWindow->getWidth() / (float)engineWindow->getHeight(), 0.001, 20);
    shaderConstant.cameraMatrix = XMMatrixMultiply(shaderConstant.cameraMatrix, mProjection);

    constantBuffer->updateData(device.getDeviceContext(), &shaderConstant);
    shader->bind(device.getDeviceContext());
    constantBuffer->bindToVertexShader(device.getDeviceContext());

    shader->draw(device.getDeviceContext(), cubeIndex, cubeVertex);

    swapChain->present(true);
}

void Renderer::loadShader() {
    std::vector<ShaderCreateInfo> shadersInfos;
    shadersInfos.push_back({ L"Shaders/VertexShader.hlsl", VERTEX_SHADER });
    shadersInfos.push_back({ L"Shaders/PixelShader.hlsl", PIXEL_SHADER });
    shader = Shader::loadShader(device.getDevice(), shadersInfos.data(), shadersInfos.size());
    std::vector< ShaderVertexInput> vertexInputs;
    vertexInputs.push_back({ "POSITION", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT });
    vertexInputs.push_back({ "COLOR", 0, sizeof(float) * 4, DXGI_FORMAT_R32G32B32A32_FLOAT });
    shader->makeInputLayout(vertexInputs.data(), vertexInputs.size());
}

void Renderer::loadCube() {
    float vertices[]{
         -1.0, -1.0,  1.0,      0.5f, 0.2f, 0.1f, 1.0f,
         1.0, -1.0,  1.0,       0.2f, 0.5f, 0.1f, 1.0f,
         1.0, -1.0, -1.0,       0.1f, 0.2f, 0.5f, 1.0f,
        -1.0, -1.0, -1.0,       0.5f, 0.1f, 0.2f, 1.0f,

        -1.0,  1.0, -1.0,      0.25f, 0.8f, 0.1f, 1.0f,
         1.0,  1.0, -1.0,      0.8f, 0.25f, 0.1f, 1.0f,
         1.0,  1.0,  1.0,      0.25f, 0.1f, 0.8f, 1.0f,
        -1.0,  1.0,  1.0,      0.1f, 0.8f, 0.25f, 1.0f,

         1.0, -1.0, -1.0,      0.9f, 0.1f, 0.2f, 1.0f,
         1.0, -1.0,  1.0,      0.1f, 0.9f, 0.2f, 1.0f,
         1.0,  1.0,  1.0,      0.9f, 0.2f, 0.1f, 1.0f,
         1.0,  1.0, -1.0,      0.2f, 0.1f, 0.9f, 1.0f,

        -1.0, -1.0,  1.0,      0.7f, 0.3f, 0.5f, 1.0f,
        -1.0, -1.0, -1.0,      0.3f, 0.7f, 0.5f, 1.0f,
        -1.0,  1.0, -1.0,      0.7f, 0.5f, 0.3f, 1.0f,
        -1.0,  1.0,  1.0,      0.5f, 0.3f, 0.5f, 1.0f,

         1.0, -1.0,  1.0,     0.12f, 0.23f, 0.4f, 1.0f,
        -1.0, -1.0,  1.0,     0.23f, 0.12f, 0.4f, 1.0f,
        -1.0,  1.0,  1.0,     0.4f, 0.23f, 0.12f, 1.0f,
         1.0,  1.0,  1.0,     0.12f, 0.4f, 0.23f, 1.0f,

        -1.0, -1.0, -1.0,     0.25f, 0.1f, 0.15f, 1.0f,
         1.0, -1.0, -1.0,     0.1f, 0.25f, 0.15f, 1.0f,
         1.0,  1.0, -1.0,     0.25f, 0.15f, 0.1f, 1.0f,
        -1.0,  1.0, -1.0,     0.15f, 0.1f, 0.1f, 1.0f
    };
    uint32_t indices[]{ 0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22 };
    cubeVertex = shader->createVertexBuffer(device.getDevice(), sizeof(vertices), 7 * sizeof(float), vertices);
    cubeIndex = shader->createIndexBuffer(device.getDevice(), indices, sizeof(indices) / sizeof(uint32_t));
}

void Renderer::release() {
    delete cubeVertex;
    delete cubeIndex;
    delete constantBuffer;
    delete shader;
    delete swapChain;
}


void Renderer::loadConstants() {
    ZeroMemory(&shaderConstant, sizeof(ShaderConstant));
    shaderConstant.worldMatrix = DirectX::XMMatrixIdentity();
    constantBuffer = new ConstantBuffer(device.getDevice(), &shaderConstant, sizeof(ShaderConstant));
}