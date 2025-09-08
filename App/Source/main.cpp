#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_internal.h"

// ---------------------------------------------------------- //
#include "Actor/Actor.h"
#include "Component/Component.h"
#include "Renderer/Renderer.h"
#include "Window/Window.h"
#include "Utilities/Utilities.h"
#include "UI/UI.h"
#include "TIme/Time.h"

// ---------------------------------------------------------- //

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	UWindow Window(1024, 1024, "Jungle Engine");

	URenderer& Renderer = URenderer::GetInstance();
	Renderer.Create(Window.GethWnd());

	UIManager& EditorUI = UIManager::Instance();
	EditorUI.Initialize(Window.GethWnd(), Renderer.GetDevice(), Renderer.GetDeviceContext());

	TimeManager& Timer = TimeManager::Instance();
	Timer.Initialize();

	bool bIsExit = false;

	// ----------------------------------------------------------------------------- //
									/* TEST CODE */
	// ----------------------------------------------------------------------------- //

	AActor CameraActor;
	CameraActor.AddComponent<UInputComponent>(&CameraActor, Window.GetKeyboard(), Window.GetMouse());
	CameraActor.AddComponent<USceneComponent>(&CameraActor, FVector(0.0f, 0.0f, -1.0f), FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
	CameraActor.AddComponent<UCameraComponent>(&CameraActor);

	Window.GetKeyboard().EnableAutoRepeat();
	// ----------------------------------------------------------------------------- //

	// ---------------------------- Temporary Timer -------------------------------- //
	auto LastTime = std::chrono::high_resolution_clock::now();
	// ----------------------------------------------------------------------------- //
	while (bIsExit == false)
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}

		Timer.Update();
		
		Renderer.Prepare();

		FMatrix M;
		FMatrix V = CameraActor.GetComponent<UCameraComponent>()->GetViewMatrix();
		FMatrix P = CameraActor.GetComponent<UCameraComponent>()->GetProjectionMatrix(Window.getAspectRatio());

		for (UObject* Obj : GUDObjectArray)
		{
			if (Obj == nullptr) 
				continue;
			AActor* Actor = static_cast<AActor*>(Obj);
			auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
			if (PrimitiveComponent)
			{
				M = Actor->GetComponent<USceneComponent>()->GetModelingMatrix();
				FMatrix MVP = M * V * P;

				PrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
					Renderer.GetDeviceContext(), 
					"constants", 
					reinterpret_cast<void*>(MVP.M));
				PrimitiveComponent->GetVertexShader()->Bind(Renderer.GetDeviceContext(), "constants");
				PrimitiveComponent->GetPixelShader()->Bind(Renderer.GetDeviceContext());
				PrimitiveComponent->Render(Renderer.GetDeviceContext());
			}
		}

		EditorUI.RenderUI();

		Renderer.SwapBuffer();
	}

	EditorUI.Release();

	return 0;
}