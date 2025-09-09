#pragma once

#include <Windows.h>

#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Window/Public/Keyboard.h"
#include "Window/Public/Mouse.h"
#include "Window/Public/WindowSettings.h"

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

	UWindow(int Width, int Height, const FString& WindowTitle);
	UWindow(const FWindowSettings& Settings);

	UWindow(const UWindow&) = delete;
	UWindow(UWindow&&) = delete;

	UWindow& operator=(const UWindow&) = delete;
	UWindow& operator=(UWindow&&) = delete;

	HWND GethWnd() const;

	UKeyboard& GetKeyboard();
	UMouse& GetMouse();
	const UKeyboard& GetKeyboard() const;
	const UMouse& GetMouse() const;

	int32 GetWidth() const;
	int32 GetHeight() const;

	void SetWindowTitle(const FString& WindowTitle);

	float getAspectRatio()
	{
		return static_cast<float>(Width) / static_cast<float>(Height);
	}
	
	bool IsResized() const { return bIsResized; }
	void ResetResizeFlag() { bIsResized = false; }
	
	void SaveWindowSettings(const FString& FilePath) const;
	void SetSettingsFilePath(const FString& FilePath);
	bool IsMaximized() const;
	
private:
	static LRESULT CALLBACK WndProcSetUp(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProcImpl(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	HWND hWnd;

	int32 Width;
	int32 Height;
	bool bIsResized = false;
	FString SettingsFilePath;
	
	// 최대화 상태 관리
	bool bWasMaximized = false;
	int32 RestoredWidth = 1024;
	int32 RestoredHeight = 1024;
	int32 RestoredPosX = 100;
	int32 RestoredPosY = 100;

	UKeyboard Keyboard;
	UMouse Mouse;
};
