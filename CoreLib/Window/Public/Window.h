#pragma once

#include <Windows.h>

#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Window/Public/Keyboard.h"
#include "Window/Public/Mouse.h"

class UWindow
{
private:
	class UWindowClass
	{
	public:
		static HINSTANCE GetInstance();
		static const WCHAR* GetWindowClassName();

		~UWindowClass();

		UWindowClass(const UWindowClass&) = delete;
		UWindowClass(UWindowClass&&) = delete;

		UWindowClass& operator=(const UWindowClass&) = delete;
		UWindowClass& operator=(UWindowClass&&) = delete;

	private:
		UWindowClass();

	private:
		static constexpr const WCHAR* WindowClassName = L"Jungle Engine";

		HINSTANCE hInstance;
	};
public:
	~UWindow();

	UWindow(int Width, int Height, const FString& WindowName);

	UWindow(const UWindow&) = delete;
	UWindow(UWindow&&) = delete;

	UWindow& operator=(const UWindow&) = delete;
	UWindow& operator=(UWindow&&) = delete;

	HWND GethWnd() const;

	UKeyboard Keyboard;
	UMouse Mouse;

	float getAspectRatio()
	{
		return static_cast<float>(Width) / static_cast<float>(Height);
	}

private:
	static LRESULT CALLBACK WndProcSetUp(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProcImpl(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	HWND hWnd;

	int Width;
	int Height;
};
