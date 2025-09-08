#pragma once

#include <ctype.h>
#include <cstring>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include "TIme/Public/TimeManager.h"
#include "Containers/Containers.h"
#include "Types/Types.h"

class UIManager
{
private:
	UIManager();
	~UIManager();

	TArray<FString> Logs;
	
	const static uint32 LogLegionSize;
public:
	// Set Imgui
	void Initialize(HWND HWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	
	void SetGreenTheme();
	
	void RenderControlPanel();
	void RenderPropertyWindow();
	void RenderConsole();
	void RenderUI();

	void Release();

	// handle log
	void AddDebugLog(FString newLog)
	{
		if (Logs.size() >= LogLegionSize)
			Logs.erase(Logs.begin());
		Logs.push_back(newLog);
	}

	void ClearLog()
	{
		Logs.clear();
	}

	// Get singleton instance
    static UIManager& Instance();
};
