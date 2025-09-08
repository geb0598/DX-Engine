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
#include "Scene/Scene.h"

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

	// 씬 매니저 초기화 및 새로운 씬 생성
	USceneManager& SceneManager = USceneManager::GetInstance();
	SceneManager.NewScene("Default Scene");

	// 메인 카메라에 입력 컴포넌트 추가
	AActor* MainCamera = SceneManager.GetMainCameraActor();
	if (MainCamera)
	{
		MainCamera->AddComponent<UInputComponent>(MainCamera, Window.GetKeyboard(), Window.GetMouse());
	}

	Window.GetKeyboard().EnableAutoRepeat();

	bool bIsExit = false;

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

		// 메인 카메라 입력 업데이트
		/*if (MainCamera && MainCamera->GetComponent<UInputComponent>())
		{
			MainCamera->GetComponent<UInputComponent>()->Update(Timer.GetDeltaTimeInSecond());
		}*/
		
		Renderer.Prepare();

		// 현재 씬에서 렌더링
		UScene* CurrentScene = SceneManager.GetCurrentScene();
		MainCamera = SceneManager.GetMainCameraActor();
		if (CurrentScene && MainCamera)
		{
			FMatrix M;
			FMatrix V = MainCamera->GetComponent<UCameraComponent>()->GetViewMatrix();
			FMatrix P = MainCamera->GetComponent<UCameraComponent>()->GetProjectionMatrix(Window.getAspectRatio());

			// 현재 씬의 모든 액터들을 렌더링
			const TArray<AActor*>& SceneActors = CurrentScene->GetActors();
			for (AActor* Actor : SceneActors)
			{
				if (Actor == nullptr) 
					continue;
					
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
		}

		EditorUI.RenderUI();

		Renderer.SwapBuffer();
	}

	EditorUI.Release();

	return 0;
}