#include "Window/Window.h"
#include "Engine/Renderer.h"


int main()
{
    auto window = Window::createWindow(800, 600, L"Lab1");
    Renderer renderer(window);

    while (!window->isNeedToClose()) {
        renderer.drawFrame();

        window->pollEvents();
    }
    renderer.release();
    return 0;
}
