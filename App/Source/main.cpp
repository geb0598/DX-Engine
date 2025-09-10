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
#include "AssetManager/AssetManager.h"
#include "Scene/Scene.h"
#include "TIme/Time.h"
#include "UI/UI.h"
#include "Utilities/Utilities.h"
#include "Window/Window.h"
#include "Line/Line.h"
#include "Grid/Grid.h"

// ------------------------------------Gizmo------------------------------------- //
#include "Component/Public/LocationGizmoComponent.h"

//[추가] 기즈모 모드 열거형
enum class EGizmoMode
{
	Location,
	Rotation,
	Max
};

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

	// #5. Resource Manager
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	ResourceManager.Initialize(Renderer.GetDevice());

	// #6. Ray Caster
	URayCaster& RayCaster = URayCaster::Instance();

	// #7. Scene Manager
	USceneManager& SceneManager = USceneManager::GetInstance();
	SceneManager.NewScene("Default Scene");

	AActor* MainCamera = SceneManager.GetMainCameraActor();
	// 기즈모를 담을 전용 액터를 생성하고 LocationGizmoComponent를 부착합니다.
	AActor* GizmoActor = new AActor();
	GizmoActor->AddComponent<ULocationGizmoComponent>(GizmoActor);
	// #7. Grid Manager
	UGridManager GridManager(Renderer.GetDevice(), Renderer.GetDeviceContext());
	GridManager.Initialize();
	GizmoActor->AddComponent<URotationGizmoComponent>(GizmoActor);

	// #8. Initialize XYZ Axis (once per application)
	ULineDrawer::InitializeXYZAxis(Renderer.GetDevice());

	//AActor* MainCamera = SceneManager.GetMainCameraActor();
	
	// ------------------------------- Axis Setup ------------------------------- //

	// TODO
	
	// ------------------------------- Misc Setup ------------------------------- //

	ImGuiIO& ImIO = ImGui::GetIO();
	// [추가] 현재 기즈모 모드를 저장할 변수 (기본값은 위치 기즈모)
	EGizmoMode CurrentGizmoMode = EGizmoMode::Location;

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

		// [추가] 키보드 이벤트를 확인하여 스페이스바가 눌렸는지 감지
		while (auto Event = Window.GetKeyboard().ReadKey())
		{
			if (Event->IsPress() && Event->GetKeyCode() == VK_SPACE)
			{
				// 다음 모드로 순환 (Location -> Rotation -> Location ...)
				int NextModeIndex = (static_cast<int>(CurrentGizmoMode) + 1) % static_cast<int>(EGizmoMode::Max);
				CurrentGizmoMode = static_cast<EGizmoMode>(NextModeIndex);
			}
		}


		// NOTE: Load Scene from here
		UScene* CurrentScene = SceneManager.GetCurrentScene();

		// NOTE: Update Main Camera
		AActor* MainCamera = SceneManager.GetMainCameraActor();

		auto CameraComponent = MainCamera->GetComponent<UCameraComponent>();
		auto CameraInputComponent = MainCamera->GetComponent<UInputComponent>();

		// TODO: Mouse and Keyboard are updated in every loop
		CameraInputComponent->SetMouse(&Window.GetMouse());
		CameraInputComponent->SetKeyboard(&Window.GetKeyboard());

		// ------------------------- Input Handling ---------------------------- //
		if (!ImIO.WantCaptureKeyboard && MainCamera && CameraInputComponent)
		{
			 CameraInputComponent->Update(Timer.GetDeltaTimeInSecond());
		}

		// ------------------------- Rendering Loop ---------------------------- //
		assert(CurrentScene && MainCamera);

		Renderer.Prepare();

		// #0. Render World Grid
		FMatrix ModelMatrix;
		FMatrix ViewMatrix = CameraComponent->GetViewMatrix();
		FMatrix ProjectionMatrix;

		if (CameraComponent->IsOrthogonal())
		{
			// TODO: These values are arbitrarily hard-coded values
			float AspectRatio = Window.GetAspectRatio();
			float Height = 20.0f;
			float Width = Height * AspectRatio;
			ProjectionMatrix = CameraComponent->GetOrthographicMatrix(
				-Width, Width, -Height, Height
			);
		}
		else
		{
			// TODO: Change name of getter to PascalCase
			ProjectionMatrix = CameraComponent->GetProjectionMatrix(Window.GetAspectRatio());
		}

		FMatrix VP = ViewMatrix * ProjectionMatrix;

		// #1. Draw World Grid once per frame (before object rendering)
		PSConstants GridInfo = {};

		GridInfo.FadeDistance = 100.0f;
		GridInfo.ScreenSize[0] = (float)Window.GetWidth();
		GridInfo.ScreenSize[1] = (float)Window.GetHeight();

		FVector CameraVector = MainCamera->GetComponent<USceneComponent>()->GetLocation();
		
		GridInfo.GridColor[0] = 0.15f;
		GridInfo.GridColor[1] = 0.15f;
		GridInfo.GridColor[2] = 0.15f;
		memcpy(GridInfo.InvViewProj, VP.Inverse().M, sizeof(GridInfo.InvViewProj));

		GridManager.Bind(GridInfo);
		GridManager.Render();

		// #1.5. Draw XYZ Axis once per frame (before object rendering)
		{
			auto& AssetManager = UAssetManager::GetInstance();
			auto VertexShader = AssetManager.GetVertexShader("DefaultVertexShader");
			auto PixelShader = AssetManager.GetPixelShader("DefaultPixelShader");

			if (VertexShader && PixelShader)
			{
				VertexShader->Bind(Renderer.GetDeviceContext(), "constants");
				PixelShader->Bind(Renderer.GetDeviceContext(), "constants");

				VertexShader->UpdateConstantBuffer(
					Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(VP.M)
				);

				// Default pixel shader constant (not selected)
				int bIsSelected = 0;
				PixelShader->UpdateConstantBuffer(
					Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(&bIsSelected)
				);

				ULineDrawer::RenderXYZAxis(Renderer.GetDeviceContext());
			}
		}

		const TArray<AActor*> SceneActors = CurrentScene->GetActors();

		// 기즈모 컴포넌트의 포인터를 가져옵니다.
		auto GizmoComponent = GizmoActor->GetComponent<ULocationGizmoComponent>();

		// [수정] 기즈모 컴포넌트 포인터를 가져오고, 현재 모드에 따라 활성화할 기즈모를 선택합니다.
		// 기즈모 입력을 먼저 처리하여, 기즈모 클릭 시 오브젝트 피킹이 무시되도록 합니다.
		auto LocationGizmo = GizmoActor->GetComponent<ULocationGizmoComponent>();
		auto RotationGizmo = GizmoActor->GetComponent<URotationGizmoComponent>();
		UGizmoComponent* ActiveGizmo = nullptr;
		switch (CurrentGizmoMode)
		{
		case EGizmoMode::Location:
			ActiveGizmo = LocationGizmo;
			break;
		case EGizmoMode::Rotation:
			ActiveGizmo = RotationGizmo;
			break;
		}

		// [수정] 활성화된 기즈모의 입력만 처리합니다.
		if (ActiveGizmo)
		{
			ActiveGizmo->HandleInput(RayCaster, Window, ViewMatrix, ProjectionMatrix);
		}

		// 기즈모가 드래그 상태가 아닐 때만 오브젝트 피킹을 수행합니다.
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
				
			}

			// 피킹 결과를 Scene과 Gizmo에 전달하고, 선택 해제까지 처리하는 통합 로직입니다.
			CurrentScene->SetCurrentActor(PickedActor);
			// [수정수정수정수정수정]
			/*GizmoComponent->SetTarget(PickedActor);*/
			if (ActiveGizmo)
			{
				ActiveGizmo->SetTarget(PickedActor);
			}

			if (PickedActor)
			{
				EditorUI.AddDebugLog("Selected Dist: " + std::to_string(PickedActorDistance));
			}
		}

		// #3. Object Rendering
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

			auto VertexShader = PrimitiveComponent->GetVertexShader();
			auto PixelShader = PrimitiveComponent->GetPixelShader();

			// NOTE: Shader Binding
			VertexShader->Bind(Renderer.GetDeviceContext(), "constants");
			PixelShader->Bind(Renderer.GetDeviceContext(), "constants");
			// PixelShader->Bind(Renderer.GetDeviceContext());

			// --------------------- Draw Primitives------------------- //

			FMatrix MVP = ModelMatrix * ViewMatrix * ProjectionMatrix;

			VertexShader->UpdateConstantBuffer(
				Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(MVP.M)
			);

			// --------------------- Highlight when clicked------------------- //

			int bIsSelected = Actor == CurrentScene->GetCurrentActor();
			PixelShader->UpdateConstantBuffer(
				Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(&bIsSelected)
			);

			// NOTE: Render Call
			PrimitiveComponent->Render(Renderer.GetDeviceContext());
		}

		// 모든 씬 액터를 렌더링한 후, 기즈모를 렌더링합니다.
		if (ActiveGizmo)
		{
			ActiveGizmo->Render(Renderer, ViewMatrix, ProjectionMatrix);
		}

		// #2. Rendering UI
		EditorUI.RenderUI();

		Renderer.SwapBuffer();
	}

	USceneManager::DestroyInstance();

	UAssetManager::GetInstance().ClearAllResources();

	GridManager.Release();

	// Release XYZ Axis resources
	ULineDrawer::ReleaseXYZAxis();

	// [추가2] 프로그램 종료 시 GizmoActor 메모리를 해제합니다.
	// NOTE: Release UI Manager
	EditorUI.Release();

	return 0;
}
