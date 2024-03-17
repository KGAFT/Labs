#include "Window.h"
#include <iostream>
#include "DXDevice.h"

DXSwapChain* swapChain = nullptr;

void resizeCallback(uint32_t width, uint32_t height) {
    if (swapChain) {
        swapChain->resize(width, height);
    }
}

int main()
{
    auto window = Window::createWindow(800, 600, L"Lab1");
    window->addResizeCallback(resizeCallback);
    DXDevice device;
    swapChain = device.getSwapChain(window);
    while (!window->isNeedToClose()) {
        swapChain->clearRenderTargets(device.getDeviceContext(), 0, 1, 0, 1);
        swapChain->bind(device.getDeviceContext(), window->getWidth(), window->getHeight());
        swapChain->present(true);
        window->pollEvents();
    }
    delete swapChain;
    
    return 0;
}
