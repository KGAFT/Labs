#include "Window/Window.h"
#include "Engine/Renderer.h"
#include <iostream>

class TestCB : public IWindowKeyCallback {
public:
    TestCB() {
        keys.push_back({ 'w', true, KEY_DOWN });
        keys.push_back({ 's', true, KEY_DOWN });
        keys.push_back({ VK_F1, false, KEY_DOWN });
    }
private:
    std::vector<WindowKey> keys;
public:
    void keyEvent(WindowKey key) override {
        if (!key.isChar) {
            std::cout << "F1" << std::endl;
        } else {
            std::cout << (char)key.key << std::endl;
        }
    }
    WindowKey* getKeys(uint32_t* pKeysAmountOut) override {
        *pKeysAmountOut = keys.size();
        return keys.data();
    }
};

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

int main()
{
    auto window = Window::createWindow(800, 600, L"Lab2");
  //  window->getInputSystem()->addKeyCallback(new TestCB);
    //window->getInputSystem()->addMouseCallback(new TestMouseCB);
    Renderer renderer(window);

    while (!window->isNeedToClose()) {
        renderer.drawFrame();

        window->pollEvents();
    }
    renderer.release();
    return 0;
}
