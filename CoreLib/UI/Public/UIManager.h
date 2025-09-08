#pragma once

#include <ctype.h>
#include <cstring>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include "TIme/Public/TimeManager.h"

class UIManager
{
private:
	UIManager();
	~UIManager();

public:
	// Set Imgui
	void Initialize(HWND HWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	
	void SetGreenTheme();
	
	void RenderControlPanel();
	void RenderPropertyWindow();
	void RenderConsole();
	void RenderUI();

	void Release();

    static UIManager& Instance();
};
