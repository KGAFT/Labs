#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <windowsx.h>
#include <dinput.h>
#include <stdexcept>

enum WindowKeyAction
{
    KEY_UP = 0,
    KEY_DOWN = 1
};

struct WindowKey
{
    uint32_t key;
    WindowKeyAction action;
};


class IWindowKeyCallback
{
public:
    virtual void keyEvent(WindowKey key) = 0;
    virtual WindowKey* getKeys(uint32_t* pKeysAmountOut) = 0;
};

class IWindowMouseCallback
{
public:
    virtual void mouseMove(uint32_t x, uint32_t y) = 0;
    virtual void mouseKey(uint32_t key) = 0;
};

class WindowInputSystem
{
    friend class Window;

public:
    WindowInputSystem(HINSTANCE instance, HWND hwnd)
    {
        if (FAILED(
            DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInputInstance, nullptr
            )))
        {
            throw std::runtime_error("Failed to create input instance");
        }
        if (FAILED(directInputInstance->CreateDevice(GUID_SysKeyboard, &keyboard, nullptr)))
        {
            throw std::runtime_error("Failed to create key board device");
        }

        HRESULT result;
        result = keyboard->SetDataFormat(&c_dfDIKeyboard);
        result = keyboard->SetCooperativeLevel(NULL, DISCL_BACKGROUND  | DISCL_EXCLUSIVE);
        result = keyboard->Acquire();
        if (FAILED(result))
        {
            throw std::runtime_error("Failed to set up keyboard");
        }
    }

private:
    std::vector<IWindowKeyCallback*> keyCallbacks;
    std::vector<IWindowMouseCallback*> mouseCallbacks;
    IDirectInput8* directInputInstance;
    IDirectInputDevice8* keyboard;
    char keyboardState[256];

public:
    void addKeyCallback(IWindowKeyCallback* keyCallback)
    {
        keyCallbacks.push_back(keyCallback);
    }

    void addMouseCallback(IWindowMouseCallback* mouseCallback)
    {
        mouseCallbacks.push_back(mouseCallback);
    }

private:
    void handlePollEvents(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        VK_ESCAPE;
        switch (msg)
        {
        case WM_MOUSEMOVE:
            checkMovementCallbacks(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            break;
        default:
            if (msg >= 0x0201 && msg <= 0x0209)
            {
                checkMouseKeyCallbacks(msg);
            }
            break;
        }
        checkKeyCallbacks();
    }

    void checkMovementCallbacks(uint32_t x, uint32_t y)
    {
        for (auto& el : mouseCallbacks)
        {
            el->mouseMove(x, y);
        }
    }

    void checkMouseKeyCallbacks(uint32_t msg)
    {
        for (auto& el : mouseCallbacks)
        {
            el->mouseKey(msg);
        }
    }

    void checkKeyCallbacks()
    {
        uint32_t keysAmount = 0;
        if (FAILED(keyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState)))
        {
            keyboard->Acquire();
            if (FAILED(keyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState)))
            {
                throw std::runtime_error("Failed to read keyboard keys");
            }
        }
        for (auto& el : keyCallbacks)
        {
            auto keys = el->getKeys(&keysAmount);
            WindowKey key;
            for (uint32_t i = 0; i < keysAmount; i++)
            {
                key = keys[i];
                if (keyboardState[key.key])
                {
                    el->keyEvent(key);
                }
            }
        }
    }
};
