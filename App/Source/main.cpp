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

// ---------------------------------------------------------------------------- //

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ------------------------------- Initial Setup ------------------------------- //
	// #1. Window Creation
	UWindow Window(1024, 1024, "Jungle Engine");

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

	// ------------------------------- Axis Setup ------------------------------- //

	// TODO
	
	// ------------------------------- Misc Setup ------------------------------- //

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
		if (Window.GetMouse().IsLeftPressed())
		{
			auto [MouseX, MouseY] = Window.GetMouse().GetPosition();

			/*
			RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, MTriangle, V, P);
			if (RayCaster.RayCastToTriangle() != DONT_INTERSECT)
				UI.AddDebugLog("Ray hit triangle");

			RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, MSphere, V, P);
			if (RayCaster.RayCastToSphere(1) != DONT_INTERSECT)
				UI.AddDebugLog("Ray hit sphere");

			RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, MCube, V, P);
			if (RayCaster.RayCastToCube() != DONT_INTERSECT)
				UI.AddDebugLog("Ray hit Cube");
			*/
		}

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
			constexpr float LEFT = -20.0f;
			constexpr float RIGHT = 20.0f;
			constexpr float BOTTOM = -20.0f;
			constexpr float TOP = 20.0f;
			ProjectionMatrix = CameraComponent->GetOrthographicMatrix(LEFT, RIGHT, BOTTOM, TOP);
		}
		else
		{
			// TODO: Change name of getter to PascalCase
			ProjectionMatrix = CameraComponent->GetProjectionMatrix(Window.getAspectRatio());
		}

		// #1. Rendering Scene Actors
		const TArray<AActor*> SceneActors = CurrentScene->GetActors();
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
			if (Window.GetMouse().IsLeftPressed())
			{
				auto [MouseX, MouseY] = Window.GetMouse().GetPosition();

				RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, ModelMatrix, ViewMatrix, ProjectionMatrix);
				if (RayCaster.RayCastToCube() != DONT_INTERSECT)
				{
					EditorUI.AddDebugLog("Clicked!");
					EditorUI.AddDebugLog("Ray hit");
				}
			}
			// ------------------------------------------------------------- //

			FMatrix MVP = ModelMatrix * ViewMatrix * ProjectionMatrix;

			auto VertexShader = PrimitiveComponent->GetVertexShader();
			auto PixelShader = PrimitiveComponent->GetPixelShader();

			VertexShader->UpdateConstantBuffer(
				Renderer.GetDeviceContext(), "constants", reinterpret_cast<void*>(MVP.M)
			);

			// NOTE: Shader Binding
			VertexShader->Bind(Renderer.GetDeviceContext(), "constants");
			PixelShader->Bind(Renderer.GetDeviceContext());

			// NOTE: Render Call
			PrimitiveComponent->Render(Renderer.GetDeviceContext());
		}

		// #2. Rendering UI
		EditorUI.RenderUI();

		Renderer.SwapBuffer();
	}

	// NOTE: Release UI Manager
	EditorUI.Release();

	return 0;
}
