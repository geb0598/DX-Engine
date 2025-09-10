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

// ---------------------------------------------------------------------------- //

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

	// #7. Grid Manager
	UGridManager GridManager(Renderer.GetDevice(), Renderer.GetDeviceContext());
	GridManager.Initialize();

	AActor* MainCamera = SceneManager.GetMainCameraActor();
	
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

		// #0. Render World Grid
		FMatrix ModelMatrix;
		FMatrix ViewMatrix = CameraComponent->GetViewMatrix();
		FMatrix ProjectionMatrix = CameraComponent->GetProjectionMatrix(Window.GetAspectRatio());

		FMatrix VP = ViewMatrix * ProjectionMatrix;

		PSConstants GridInfo = {};

		GridInfo.FadeDistance = 1000.0f;
		GridInfo.ScreenSize[0] = (float)Window.GetWidth();
		GridInfo.ScreenSize[1] = (float)Window.GetHeight();
		
		GridInfo.GridColor[0] = 1.0f;
		GridInfo.GridColor[1] = 0.0f;
		GridInfo.GridColor[2] = 1.0f;
		memcpy(GridInfo.InvViewProj, VP.Inverse().M, sizeof(GridInfo.InvViewProj));

		GridManager.Bind(GridInfo);
		GridManager.Render();

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

		const TArray<AActor*> SceneActors = CurrentScene->GetActors();

		// #1. Object Picking
		AActor* PickedActor = nullptr;
		float PickedActorDistance = (std::numeric_limits<float>::max)();

		// TODO: Improve performance with caching
		if (!ImIO.WantCaptureMouse && Window.GetMouse().IsLeftPressed())
		{
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

				// --------------------- Object Picking------------------------- //
				auto [MouseX, MouseY] = Window.GetMouse().GetPosition();

				auto HitResult = PrimitiveComponent->GetHitResultAtScreenPosition(
					RayCaster,
					MouseX,
					MouseY,
					Window.GetWidth(),
					Window.GetHeight(),
					ModelMatrix,
					ViewMatrix,
					ProjectionMatrix
				);

				if (HitResult && PickedActorDistance > *HitResult)
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
				// ----------------------------------------------------------- //

			}

			if (PickedActor)
			{
				CurrentScene->SetCurrentActor(PickedActor);
				EditorUI.AddDebugLog("Selected Dist: " + std::to_string(PickedActorDistance));
			}
		}

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

			auto VertexShader = PrimitiveComponent->GetVertexShader();
			auto PixelShader = PrimitiveComponent->GetPixelShader();

			// NOTE: Shader Binding
			VertexShader->Bind(Renderer.GetDeviceContext(), "constants");
			PixelShader->Bind(Renderer.GetDeviceContext(), "constants");
			// PixelShader->Bind(Renderer.GetDeviceContext());

			// --------------------- Draw XYZ Axis first-------------------- //

			VertexShader->UpdateConstantBuffer(
				Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(VP.M)
			);

			ULineDrawer::RenderXYZAxis(Renderer.GetDevice(), Renderer.GetDeviceContext());

			// --------------------- Then Draw Primitives------------------- //

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

		// #2. Rendering UI
		EditorUI.RenderUI();

		Renderer.SwapBuffer();
	}

	GridManager.Release();

	// NOTE: Release UI Manager
	EditorUI.Release();

	return 0;
}
