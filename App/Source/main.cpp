// ------------------------------- C++ Headers -------------------------------- //
#include <limits>

// ----------------------------- ImGui Headers -------------------------------- //
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_internal.h"

// --------------------------------- Modules ---------------------------------- //
#include "Actor/Actor.h"
#include "Component/Component.h"
#include "RayCaster/Raycaster.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "TIme/Time.h"
#include "UI/UI.h"
#include "Utilities/Utilities.h"
#include "Window/Window.h"

// ------------------------------------Gizmo------------------------------------- //
#include "Component/Public/LocationGizmoComponent.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ------------------------------- Initial Setup ------------------------------- //
	// #0. Logger Add File Output
	CLogger::Instance().AddOutput(std::make_unique<CFileOutput>("JungleEngine.log"));
	UE_LOG(Info, "Hello World %d", 2025);

	// #1. Window Creation with Settings
	FWindowSettings windowSettings;
	const FString settingsPath = "WindowSettings.ini";
	
	// #1.1 If Load File Exists, Load Settings
	if (!windowSettings.LoadFromFile(settingsPath))
	{
		// # If Load File Not Exists, Create with Default Settings
		windowSettings.Width = 1600;
		windowSettings.Height = 900;
		windowSettings.WindowTitle = "Jungle Engine";
		windowSettings.bIsMaximized = false;
	}
	
	// #1.2 Set SettingsFilePath to Window
	UWindow Window(windowSettings);
	Window.SetSettingsFilePath(settingsPath);

	// #2. Renderer
	URenderer& Renderer = URenderer::GetInstance();
	// NOTE: Renderer should be initialized before use
	Renderer.Create(Window.GethWnd());

	// #3. UI Manager
	UIManager& EditorUI = UIManager::Instance();
	// NOTE: UIManager should be initialized before use
	EditorUI.Initialize(Window.GethWnd(), Renderer.GetDevice(), Renderer.GetDeviceContext());
	// TODO: Should follow Naming Convention

	// #4. Time Manager
	TimeManager& Timer = TimeManager::Instance();
	Timer.Initialize();
	// TODO: Should follow Naming Convention

	// #5. Ray Caster
	URayCaster& RayCaster = URayCaster::Instance();

	// #6. Scene Manager
	USceneManager& SceneManager = USceneManager::GetInstance();
	SceneManager.NewScene("Default Scene");

	AActor* MainCamera = SceneManager.GetMainCameraActor();
	// [추가] 기즈모를 담을 전용 액터를 생성하고 LocationGizmoComponent를 부착합니다.
	// 이 액터는 씬에 추가하지 않습니다. 컴포넌트가 직접 렌더링을 관리하기 때문입니다.
	AActor* GizmoActor = new AActor();
	GizmoActor->AddComponent<ULocationGizmoComponent>(GizmoActor);

	// ------------------------------- Axis Setup ------------------------------- //

	// TODO
	
	// ------------------------------- Misc Setup ------------------------------- //

	ImGuiIO& ImIO = ImGui::GetIO();

	// -------------------------------- Main Loop -------------------------------- //
	bool bIsExit = false;
	while (bIsExit == false)
	{
		// ------------------------- Message Loop  ----------------------------- //
		MSG Msg;
		while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);

			if (Msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}

		// ------------------------- Window Resize Handling -------------------- //
		if (Window.IsResized())
		{
			Renderer.ResizeBuffers(Window.GetWidth(), Window.GetHeight());
			Window.ResetResizeFlag();
		}

		// ------------------------- Scene Load ------------------------------- //
		Timer.Update();

		// NOTE: Load Scene from here
		UScene* CurrentScene = SceneManager.GetCurrentScene();

		// NOTE: Update Main Camera
		MainCamera = SceneManager.GetMainCameraActor();

		auto CameraComponent = MainCamera->GetComponent<UCameraComponent>();
		auto CameraInputComponent = MainCamera->GetComponent<UInputComponent>();

		// TODO: Mouse and Keyboard are updated in every loop
		CameraInputComponent->SetMouse(&Window.GetMouse());
		CameraInputComponent->SetKeyboard(&Window.GetKeyboard());

		// ------------------------- Input Handling ---------------------------- //
		if (MainCamera && CameraInputComponent)
		{
			 CameraInputComponent->Update(Timer.GetDeltaTimeInSecond());
		}

		// ------------------------- Rendering Loop ---------------------------- //
		assert(CurrentScene && MainCamera);

		Renderer.Prepare();

		FMatrix ModelMatrix;
		FMatrix ViewMatrix = CameraComponent->GetViewMatrix();
		FMatrix ProjectionMatrix;

		if (CameraComponent->IsOrthogonal())
		{
			// TODO: These values are arbitrarily hard-coded values
			float AspectRatio = Window.getAspectRatio();
			float Height = 20.0f;
			float Width = Height * AspectRatio;
			ProjectionMatrix = CameraComponent->GetOrthographicMatrix(
				-Width, Width, -Height, Height
			);
		}
		else
		{
			// TODO: Change name of getter to PascalCase
			ProjectionMatrix = CameraComponent->GetProjectionMatrix(Window.getAspectRatio());
		}

		const TArray<AActor*> SceneActors = CurrentScene->GetActors();

		// [추가1] 기즈모 컴포넌트의 포인터를 가져옵니다.
		auto GizmoComponent = GizmoActor->GetComponent<ULocationGizmoComponent>();

		//// #1. Object Picking
		//AActor* PickedActor = nullptr;
		//float PickedActorDistance = (std::numeric_limits<float>::max)();

		if (!ImIO.WantCaptureMouse && Window.GetMouse().IsLeftPressed() && !GizmoComponent->IsDragging())
		{
			AActor* PickedActor = nullptr;
			float PickedActorDistance = (std::numeric_limits<float>::max)();

			for (auto Actor : SceneActors)
			{
				if (Actor == nullptr) continue;
				auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
				if (PrimitiveComponent == nullptr) continue;
				auto SceneComponent = Actor->GetComponent<USceneComponent>();
				if (SceneComponent == nullptr) continue;

				ModelMatrix = SceneComponent->GetModelingMatrix();
				auto [MouseX, MouseY] = Window.GetMouse().GetPosition();
				auto HitResult = PrimitiveComponent->GetHitResultAtScreenPosition(RayCaster, MouseX, MouseY, Window.GetWidth(), Window.GetHeight(), ModelMatrix, ViewMatrix, ProjectionMatrix);

				if (HitResult && *HitResult < PickedActorDistance)
				{
					PickedActor = Actor;
					PickedActorDistance = *HitResult;
				}
				// ------------------------- Debug ---------------------------- //
				if (!HitResult)
				{
					continue;
				}

				switch (PrimitiveComponent->GetType())
				{
				case UPrimitiveComponent::EType::Triangle:
					EditorUI.AddDebugLog("Triangle Hit!");
					EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
					break;
				case UPrimitiveComponent::EType::Cube:
					EditorUI.AddDebugLog("Cube Hit!");
					EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
					break;
				case UPrimitiveComponent::EType::Sphere:
					EditorUI.AddDebugLog("Sphere Hit!");
					EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
					break;
				default:
					EditorUI.AddDebugLog("Unknown Hit!");
					EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
					break;
				}
				// [삭제] 기존 디버그 로그는 피킹 로직과 분리되어 있어 삭제합니다. (필요시 UIManager에서 확인 가능)
			}

			// [추가1] 피킹 결과를 Scene과 Gizmo에 전달하고, 선택 해제까지 처리하는 통합 로직입니다.
			CurrentScene->SetCurrentActor(PickedActor);
			GizmoComponent->SetTarget(PickedActor);

			if (PickedActor)
			{
				EditorUI.AddDebugLog("Selected Dist: " + std::to_string(PickedActorDistance));
			}
		}
		// [추가1] 기즈모 입력(드래그) 처리를 호출합니다.
		GizmoComponent->HandleInput(RayCaster, Window, ViewMatrix, ProjectionMatrix);

		//// TODO: Improve performance with caching
		//if (!ImIO.WantCaptureMouse && Window.GetMouse().IsLeftPressed())
		//{
		//	for (auto Actor : SceneActors)
		//	{
		//		if (Actor == nullptr)
		//		{
		//			continue;
		//		}

		//		auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
		//		if (PrimitiveComponent == nullptr)
		//		{
		//			continue;
		//		}

		//		// NOTE: Actor with no scene component is ignored quietely
		//		auto SceneComponent = Actor->GetComponent<USceneComponent>();
		//		if (SceneComponent == nullptr)
		//		{
		//			continue;
		//		}
		//		ModelMatrix = SceneComponent->GetModelingMatrix();

		//		// --------------------- Object Picking------------------------- //
		//		auto [MouseX, MouseY] = Window.GetMouse().GetPosition();

		//		auto HitResult = PrimitiveComponent->GetHitResultAtScreenPosition(
		//			RayCaster,
		//			MouseX,
		//			MouseY,
		//			Window.GetWidth(),
		//			Window.GetHeight(),
		//			ModelMatrix,
		//			ViewMatrix,
		//			ProjectionMatrix
		//		);

		//		if (HitResult && PickedActorDistance > *HitResult)
		//		{
		//			PickedActor = Actor;
		//			PickedActorDistance = *HitResult;
		//		}

		//		// ------------------------- Debug ---------------------------- //
		//		if (!HitResult)
		//		{
		//			continue;
		//		}

		//		switch (PrimitiveComponent->GetType())
		//		{
		//		case UPrimitiveComponent::EType::Triangle:
		//			EditorUI.AddDebugLog("Triangle Hit!");
		//			EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
		//			break;
		//		case UPrimitiveComponent::EType::Cube:
		//			EditorUI.AddDebugLog("Cube Hit!");
		//			EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
		//			break;
		//		case UPrimitiveComponent::EType::Sphere:
		//			EditorUI.AddDebugLog("Sphere Hit!");
		//			EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
		//			break;
		//		default:
		//			EditorUI.AddDebugLog("Unknown Hit!");
		//			EditorUI.AddDebugLog("Dist: " + std::to_string(*HitResult));
		//			break;
		//		}
		//		// ----------------------------------------------------------- //

		//	}

		//	if (PickedActor)
		//	{
		//		CurrentScene->SetCurrentActor(PickedActor);
		//		EditorUI.AddDebugLog("Selected Dist: " + std::to_string(PickedActorDistance));
		//	}
		//}

		// #2. Object Rendering
		for (auto Actor : SceneActors)
		{
			if (Actor == nullptr)
			{
				continue;
			}

			auto PrimitiveComponent = Actor->GetComponent<UPrimitiveComponent>();
			if (PrimitiveComponent == nullptr)
			{
				continue;
			}

			// NOTE: Actor with no scene component is ignored quietely
			auto SceneComponent = Actor->GetComponent<USceneComponent>();
			if (SceneComponent == nullptr)
			{
				continue;
			}
			ModelMatrix = SceneComponent->GetModelingMatrix();

			FMatrix MVP = ModelMatrix * ViewMatrix * ProjectionMatrix;

			auto VertexShader = PrimitiveComponent->GetVertexShader();
			auto PixelShader = PrimitiveComponent->GetPixelShader();

			/*VertexShader->UpdateConstantBuffer(
				Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(MVP.M)
			);

			int bIsSelected = Actor == CurrentScene->GetCurrentActor();
			PixelShader->UpdateConstantBuffer(
				Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(&bIsSelected)
			);*/

			// NOTE: Shader Binding
			VertexShader->Bind(Renderer.GetDeviceContext(), "constants");
			PixelShader->Bind(Renderer.GetDeviceContext(), "constants");
			// PixelShader->Bind(Renderer.GetDeviceContext());

			// NOTE: Render Call
			PrimitiveComponent->Render(Renderer.GetDeviceContext());
		}

		// [추가2] 모든 씬 액터를 렌더링한 후, 기즈모를 렌더링합니다.
		GizmoComponent->Render(Renderer, ViewMatrix, ProjectionMatrix);

		// #2. Rendering UI
		EditorUI.RenderUI();

		Renderer.SwapBuffer();
	}

	// [추가2] 프로그램 종료 시 GizmoActor 메모리를 해제합니다.
	delete GizmoActor;
	// NOTE: Release UI Manager
	EditorUI.Release();

	return 0;
}
