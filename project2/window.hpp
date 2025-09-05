#pragma once  

#include <windows.h>  

// Update the include paths to match the correct relative or absolute path to ImGui headers  
#include "../ImGui/imgui.h"  
#include "../ImGui/imgui_internal.h"  
#include "../ImGui/imgui_impl_dx11.h"  
#include "../ImGui/imgui_impl_win32.h"  

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);
// 각종 메시지를 처리할 함수  
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);