#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>
#include "WindowInputSystem.h"

class Window
{
public:
	static Window* createWindow(HINSTANCE instance, uint32_t width, uint32_t height, const wchar_t* windowTitle);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	Window(HINSTANCE instance);
private:
	HWND windowHandle;
	HINSTANCE instance;
	uint32_t width;
	uint32_t height;
	const wchar_t* title;
	bool needToClose = false;
	WindowInputSystem* inputSystem;
	std::vector<void(*)(uint32_t, uint32_t)> resizeCallbacks;
	bool windowReady = false;
public:
	void pollEvents();
	void addResizeCallback(void(*resizeCallback)(uint32_t, uint32_t));
	bool isNeedToClose();
	HWND getWindowHandle();
	uint32_t getWidth();
	uint32_t getHeight();
	WindowInputSystem* getInputSystem();
private:
	void checkResizeCallbacks();
	void checkEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

