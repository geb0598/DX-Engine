#pragma once

#include "ImGui/imgui_impl_win32.h"

#include "Window/Public/Window.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

HINSTANCE UWindow::UWindowClass::GetInstance()
{
	static UWindowClass WindowClass;
	return WindowClass.hInstance;
}

const WCHAR* UWindow::UWindowClass::GetWindowClassName()
{
	return WindowClassName;
}

UWindow::UWindowClass::~UWindowClass()
{
	UnregisterClassW(GetWindowClassName(), GetInstance());
}

UWindow::UWindowClass::UWindowClass()
	: hInstance(GetModuleHandle(nullptr))
{
	WNDCLASSEXW WndClass = {};
	WndClass.cbSize = sizeof(WNDCLASSEXW);
	WndClass.style = CS_OWNDC;
	WndClass.lpfnWndProc = WndProcSetUp;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = nullptr;
	WndClass.hCursor = nullptr;
	WndClass.hbrBackground = nullptr;
	WndClass.lpszMenuName = nullptr;
	WndClass.lpszClassName = GetWindowClassName();
	WndClass.hIconSm = nullptr;

	RegisterClassExW(&WndClass);
}

UWindow::~UWindow()
{
	DestroyWindow(hWnd);
}

UWindow::UWindow(int Width, int Height, const FString& WindowName)
	: Width(Width), Height(Height)
{
	hWnd = CreateWindowW(
		UWindowClass::GetWindowClassName(),
		std::wstring(WindowName.begin(), WindowName.end()).c_str(),
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, 
		CW_USEDEFAULT,
		Width, 
		Height,
		nullptr, 
		nullptr, 
		UWindowClass::GetInstance(), 
		this
	);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
}

HWND UWindow::GethWnd() const
{
	return hWnd;
}

LRESULT UWindow::WndProcSetUp(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* Create = reinterpret_cast<CREATESTRUCTW*>(lParam);

		UWindow* const Window = static_cast<UWindow*>(Create->lpCreateParams);

		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(Window));
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&UWindow::WndProc));

		return Window->WndProcImpl(hWnd, Msg, wParam, lParam);
	}

	return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

LRESULT UWindow::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	UWindow* Window = reinterpret_cast<UWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return Window->WndProcImpl(hWnd, Msg, wParam, lParam);
}

LRESULT UWindow::WndProcImpl(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{

	if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
	{
		return true;
	}

	switch (Msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_KILLFOCUS:
		Keyboard.ClearState();
		break;
	case WM_KEYDOWN:
		[[fallthrough]];
	case WM_SYSKEYDOWN:
		if (!(lParam & 0x40000000) || Keyboard.IsAutoRepeatEnabled())
		{
			Keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
		[[fallthrough]];
	case WM_SYSKEYUP:
		Keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		Keyboard.OnChar(static_cast<unsigned char>(wParam));
		break;
	case WM_MOUSEMOVE:
	{
		const POINTS Points = MAKEPOINTS(lParam);
		if (0 <= Points.x && Points.x < Width && 0 <= Points.y && Points.y < Height)
		{
			Mouse.OnMouseMove(Points.x, Points.y);
			if (!Mouse.IsInsideWindow())
			{
				SetCapture(hWnd);
				Mouse.OnMouseEnter();
			}
		}
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				Mouse.OnMouseMove(Points.x, Points.y);
			}
			else
			{
				ReleaseCapture();
				Mouse.OnMouseLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		SetForegroundWindow(hWnd);
		const POINTS Points = MAKEPOINTS(lParam);
		Mouse.OnLeftPressed(Points.x, Points.y);
		break;
	}
	case WM_LBUTTONUP:
	{
		const POINTS Points = MAKEPOINTS(lParam);
		Mouse.OnLeftReleased(Points.x, Points.y);
		if (!(0 <= Points.x && Points.x < Width && 0 <= Points.y && Points.y < Height))
		{
			ReleaseCapture();
			Mouse.OnMouseLeave();
		}
		break;
	}
	case WM_RBUTTONDOWN:
	{
		const POINTS Points = MAKEPOINTS(lParam);
		Mouse.OnRightPressed(Points.x, Points.y);
		break;
	}
	case WM_RBUTTONUP:
	{
		const POINTS Points = MAKEPOINTS(lParam);
		Mouse.OnRightReleased(Points.x, Points.y);
		if (!(0 <= Points.x && Points.x < Width && 0 <= Points.y && Points.y < Height))
		{
			ReleaseCapture();
			Mouse.OnMouseLeave();
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		const POINTS Points = MAKEPOINTS(lParam);
		int Delta = GET_WHEEL_DELTA_WPARAM(wParam);
		Mouse.OnWheelDelta(Points.x, Points.y, Delta);
		break;
	}
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}
