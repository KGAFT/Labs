#pragma once

#include <Windows.h>
#include <cstdint>
#include <vector>

class Window
{
public:
	static Window* createWindow(uint32_t width, uint32_t height, const wchar_t* windowTitle);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	Window();
private:
	HWND windowHandle;
	uint32_t width;
	uint32_t height;
	const wchar_t* title;
	bool needToClose = false;
	std::vector<void(*)(uint32_t, uint32_t)> resizeCallbacks;
public:
	void pollEvents();
	void addResizeCallback(void(*resizeCallback)(uint32_t, uint32_t));
	bool isNeedToClose();
	HWND getWindowHandle();
	uint32_t getWidth();
	uint32_t getHeight();
private:
	void checkResizeCallbacks();
};

