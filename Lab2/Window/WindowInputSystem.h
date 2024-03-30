#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <windowsx.h>

enum WindowKeyAction {
	KEY_UP,
	KEY_DOWN
};

struct WindowKey {
	uint32_t key;
	bool isChar;
	WindowKeyAction action;
};


class IWindowKeyCallback {
public:
	virtual void keyEvent(WindowKey key) = 0;
	virtual WindowKey* getKeys(uint32_t* pKeysAmountOut) = 0;
};

class IWindowMouseCallback {
public:
	virtual void mouseMove(uint32_t x, uint32_t y) = 0;
	virtual void mouseKey(uint32_t key) = 0;
};

class WindowInputSystem
{
	friend class Window;
public:
	WindowInputSystem(){}
private:
	std::vector<IWindowKeyCallback*> keyCallbacks;
	std::vector<IWindowMouseCallback*> mouseCallbacks;
public:
	void addKeyCallback(IWindowKeyCallback* keyCallback) {
		keyCallbacks.push_back(keyCallback);
	}
	void addMouseCallback(IWindowMouseCallback* mouseCallback) {
		mouseCallbacks.push_back(mouseCallback);
	}
private:
	void handlePollEvents(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		VK_ESCAPE;
		switch (msg)
		{
		case WM_KEYDOWN:
			checkKeyCallbacks(wparam, false, WindowKeyAction::KEY_DOWN);
			break;
		case WM_KEYUP:
			checkKeyCallbacks(wparam, false, WindowKeyAction::KEY_UP);
			break;
		case WM_CHAR:
			checkKeyCallbacks(wparam, true, WindowKeyAction::KEY_DOWN);
			break;
		case WM_MOUSEMOVE:
			checkMovementCallbacks(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			break;
		default:
			if (msg >= 0x0201 && msg <= 0x0209) {
				checkMouseKeyCallbacks(msg);
			}
			break;
		}
	}

	void checkMovementCallbacks(uint32_t x, uint32_t y) {
		for (auto& el : mouseCallbacks) {
			el->mouseMove(x, y);
		}
	}

	void checkMouseKeyCallbacks(uint32_t msg) {
		for (auto& el : mouseCallbacks) {
			el->mouseKey(msg);
		}
	}

	void checkKeyCallbacks(WPARAM param, bool isChar, WindowKeyAction action) {
		uint32_t keysAmount = 0;
		for (auto& el : keyCallbacks) {
			auto keys = el->getKeys(&keysAmount);
			for (uint32_t i = 0; i < keysAmount; i++) {
				if (keys[i].isChar == isChar && keys[i].action==action && keys[i].key == param) {
					el->keyEvent(keys[i]);
				}
			}
		}
	}
};

