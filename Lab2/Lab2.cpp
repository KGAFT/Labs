#include "Window/Window.h"
#include "Engine/Renderer.h"
#include <iostream>


class TestMouseCB : public IWindowMouseCallback {
public:
    void mouseMove(uint32_t x, uint32_t y) override {
        std::cout << "X: " << x << " Y: " << y << std::endl;
    }
     void mouseKey(uint32_t key) {
        if (key == WM_LBUTTONDOWN) {
            std::cout << "LEFT BUTTON" << std::endl;
        }
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, int nCmdShow)
{
    auto window = Window::createWindow(hInstance, 1920, 1080, L"Lab2");
    Renderer renderer(window);

    while (!window->isNeedToClose()) {
        renderer.drawFrame();

        window->pollEvents();
    }
    renderer.release();
    return 0;
}

