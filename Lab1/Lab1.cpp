#include "Window.h"
#include <iostream>
#include "DXDevice.h"
#include "Shader.h"
#include "ConstantBuffer.h"
DXSwapChain* swapChain = nullptr;

void resizeCallback(uint32_t width, uint32_t height) {
    if (swapChain) {
        swapChain->resize(width, height);
    }
}

struct TestConstant {
    alignas(16) float col;
};

int main()
{
    auto window = Window::createWindow(800, 600, L"Lab1");
    window->addResizeCallback(resizeCallback);
    DXDevice device;
    swapChain = device.getSwapChain(window);

    std::vector<ShaderCreateInfo> shadersInfos;
    shadersInfos.push_back({ L"C:/Users/kgaft/Documents/Labs/Lab1/x64/Debug/VertexShader.hlsl", VERTEX_SHADER });
    shadersInfos.push_back({ L"C:/Users/kgaft/Documents/Labs/Lab1/x64/Debug/PixelShader.hlsl", PIXEL_SHADER });
    Shader* shader = Shader::loadShader(device.getDevice(), shadersInfos.data(), shadersInfos.size());
    std::vector< ShaderVertexInput> vertexInputs;
    vertexInputs.push_back({ "POSITION", 0, sizeof(float) * 3, DXGI_FORMAT_R32G32B32_FLOAT });
    vertexInputs.push_back({ "COLOR", 0, sizeof(float) * 4, DXGI_FORMAT_R32G32B32A32_FLOAT });
    shader->makeInputLayout(vertexInputs.data(), vertexInputs.size());


    float vertices[]{
        -0.5,-0.5, 0.0 ,    1,0,0,1,
        -0.5, 0.5, 0.0,      0,1,0,1,
        0.5, -0.5, 0.0,       0,0,1,1,
        0.5, 0.5, 0.0,         1,0,0,1
    };
    uint32_t indices[]{ 0,1,2, 1, 3, 2};
    auto vBuffer = shader->createVertexBuffer(device.getDevice(), sizeof(vertices), 7 * sizeof(float), vertices);
    auto iBuffer = shader->createIndexBuffer(device.getDevice(), indices, 6);
    TestConstant constant = { 0.00001f };
    ConstantBuffer cb(device.getDevice(), &constant, sizeof(TestConstant));

    while (!window->isNeedToClose()) {
        swapChain->clearRenderTargets(device.getDeviceContext(), 0.1f, 0.2f, 0.5f, 1.0f);
        swapChain->bind(device.getDeviceContext(), window->getWidth(), window->getHeight());
        shader->bind(device.getDeviceContext());
        cb.bindToPixelShader(device.getDeviceContext());
        shader->draw(device.getDeviceContext(), iBuffer, vBuffer);

        swapChain->present(true);
        constant.col += 0.01;
        cb.updateData(device.getDeviceContext(), &constant);

        window->pollEvents();
    }
    delete swapChain;
    
    return 0;
}
