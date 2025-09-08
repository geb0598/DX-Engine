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

	UIManager &UI = UIManager::Instance();
	UI.Initialize(Window.GethWnd(), Renderer.GetDevice(), Renderer.GetDeviceContext());

	URayCaster& RayCaster = URayCaster::Instance();

	TimeManager& Timer = TimeManager::Instance();
	Timer.Initialize();

	// 씬 매니저 초기화 및 새로운 씬 생성
	USceneManager& SceneManager = USceneManager::GetInstance();
	SceneManager.NewScene("Default Scene");

	// 메인 카메라에 입력 컴포넌트 추가
	AActor* MainCamera = SceneManager.GetMainCameraActor();
	Window.GetKeyboard().EnableAutoRepeat();

	bool bIsExit = false;

	// ----------------------------------------------------------------------------- //
									/* TEST CODE */
	// ----------------------------------------------------------------------------- //

	// allocate shader and input layout first
	TArray<D3D11_INPUT_ELEMENT_DESC> InputLayoutDesc(
		std::begin(FVertex::InputLayoutDesc),
		std::end(FVertex::InputLayoutDesc)
	);

	std::shared_ptr<UVertexShader> VertexShader = std::make_shared<UVertexShader>(
		Renderer.GetDevice(),
		"./Shader/VertexShader.hlsl",
		"main",
		InputLayoutDesc
	);

	std::shared_ptr<UPixelShader> PixelShader = std::make_shared<UPixelShader>(
		Renderer.GetDevice(),
		"./Shader/PixelShader.hlsl",
		"main"
	);

	TArray<FVertex> XAxisVertexArray;
	for (size_t i = 0; i < sizeof(XAxisVertices) / sizeof(FVertexSimple); ++i)
	{
		XAxisVertexArray.push_back(static_cast<FVertex>(XAxisVertices[i]));
	}

	TArray<FVertex> YAxisVertexArray;
	for (size_t i = 0; i < sizeof(YAxisVertices) / sizeof(FVertexSimple); ++i)
	{
		YAxisVertexArray.push_back(static_cast<FVertex>(YAxisVertices[i]));
	}

	TArray<FVertex> ZAxisVertexArray;
	for (size_t i = 0; i < sizeof(ZAxisVertices) / sizeof(FVertexSimple); ++i)
	{
		ZAxisVertexArray.push_back(static_cast<FVertex>(ZAxisVertices[i]));
	}

	UAxisDrawer XAxisDrawer(Renderer.GetDevice(), XAxisVertexArray);
	UAxisDrawer YAxisDrawer(Renderer.GetDevice(), YAxisVertexArray);
	UAxisDrawer ZAxisDrawer(Renderer.GetDevice(), ZAxisVertexArray);

	AActor CameraActor;
	CameraActor.AddComponent<USceneComponent>(&CameraActor, FVector(0.0f, 0.0f, -1.0f), FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
	CameraActor.AddComponent<UCameraComponent>(&CameraActor);

	FMatrix V = CameraActor.GetComponent<UCameraComponent>()->GetViewMatrix();
	FMatrix P = CameraActor.GetComponent<UCameraComponent>()->GetProjectionMatrix(Window.getAspectRatio());

	AActor TriangleActor;

	TArray<FVertex> VertexArray;
	for (size_t i = 0; i < sizeof(triangle_vertices) / sizeof(FVertexSimple); ++i)
	{
		VertexArray.push_back(static_cast<FVertex>(triangle_vertices[i]));
	}

	std::shared_ptr<UMesh> TriangleMesh = std::make_shared<UMesh>(Renderer.GetDevice(), VertexArray);

	TriangleActor.AddComponent<UPrimitiveComponent>(&TriangleActor, TriangleMesh, VertexShader, PixelShader);
	TriangleActor.AddComponent<USceneComponent>(&TriangleActor, FVector(0.5f, -0.8f, 50.0f), FVector(20.0f, 30.0f, -50.0f), FVector(0.5f, 1.2f, 1.4f));

	FMatrix MTriangle = TriangleActor.GetComponent<USceneComponent>()->GetModelingMatrix();

	AActor SphereActor;

	TArray<FVertex> SphereVertexArray;
	for (size_t i = 0; i < sizeof(sphere_vertices) / sizeof(FVertexSimple); ++i)
	{
		SphereVertexArray.push_back(static_cast<FVertex>(sphere_vertices[i]));
	}

	std::shared_ptr<UMesh> SphereMesh = std::make_shared<UMesh>(Renderer.GetDevice(), SphereVertexArray);

	SphereActor.AddComponent<UPrimitiveComponent>(&SphereActor, SphereMesh, VertexShader, PixelShader);
	SphereActor.AddComponent<USceneComponent>(&SphereActor, FVector(3.0f, -2.0f, 50.0f), FVector(7.0f, -20.0f, -90.0f), FVector(2.0f, 1.0f, 0.5f));

	FMatrix MSphere = SphereActor.GetComponent<USceneComponent>()->GetModelingMatrix();

	AActor CubeActor;

	TArray<FVertex> CubeVertexArray;
	for (size_t i = 0; i < sizeof(cube_vertices) / sizeof(FVertexSimple); ++i)
	{
		CubeVertexArray.push_back(static_cast<FVertex>(cube_vertices[i]));
	}

	std::shared_ptr<UMesh> CubeMesh = std::make_shared<UMesh>(Renderer.GetDevice(), CubeVertexArray);

	CubeActor.AddComponent<UPrimitiveComponent>(&CubeActor, CubeMesh, VertexShader, PixelShader);
	CubeActor.AddComponent<USceneComponent>(&CubeActor, FVector(-3.0f, 2.0f, 50.0f), FVector(-7.0f, 20.0f, 9.0f), FVector(-2.0f, -1.0f, -0.5f));

	FMatrix MCube = CubeActor.GetComponent<USceneComponent>()->GetModelingMatrix();
	
	// ----------------------------------------------------------------------------- //

	while (bIsExit == false)
	{
		// test raytracer
		if (Window.Mouse.IsLeftPressed())
		{
			int32 MouseX = Window.Mouse.GetXPosition();
			int32 MouseY = Window.Mouse.GetYPosition();

			// mouse position
			/*UI.AddDebugLog("Mouse clicked!");
			UI.AddDebugLog("X : " + std::to_string(MouseX) + " Y : " + std::to_string(MouseY));*/

			// ndc mouse position
			/*float NDCX = (static_cast<float>(MouseX) - 500.0f) / 500.0f;
			float NDCY = (static_cast<float>(MouseY) - 500.0f) / 500.0f;

			UI.AddDebugLog("NDCX : " + std::to_string(NDCX));
			UI.AddDebugLog("NDCY : " + std::to_string(NDCY));*/

			RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, MTriangle, V, P);
			if (RayCaster.RayCastToTriangle() != DONT_INTERSECT)
				UI.AddDebugLog("Ray hit triangle");

			RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, MSphere, V, P);
			if (RayCaster.RayCastToSphere(1) != DONT_INTERSECT)
				UI.AddDebugLog("Ray hit sphere");

			RayCaster.SetRayWithMouseAndMVP(MouseX, MouseY, MCube, V, P);
			if (RayCaster.RayCastToCube() != DONT_INTERSECT)
				UI.AddDebugLog("Ray hit Cube");
		}

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
		
		Renderer.Prepare();

		// 현재 씬에서 렌더링
		UScene* CurrentScene = SceneManager.GetCurrentScene();
		MainCamera = SceneManager.GetMainCameraActor();
		MainCamera->GetComponent<UInputComponent>()->SetMouse(&Window.GetMouse());
		MainCamera->GetComponent<UInputComponent>()->SetKeyboard(&Window.GetKeyboard());
		if (MainCamera && MainCamera->GetComponent<UInputComponent>())
		{
			MainCamera->GetComponent<UInputComponent>()->Update(Timer.GetDeltaTimeInSecond());
		}

		if (CurrentScene && MainCamera)
		{
			auto CameraComponent = MainCamera->GetComponent<UCameraComponent>();
			FMatrix M;
			FMatrix V = CameraComponent->GetViewMatrix();
			FMatrix P;
			if (CameraComponent->IsOrthogonal())
			{
				P = CameraComponent->GetOrthographicMatrix(-30.0f, 30.0f, -30.0f, 30.0f);
			}
			else
			{
				P = CameraComponent->GetProjectionMatrix(Window.getAspectRatio());
			}

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

		MVP = MSphere * V * P;

		auto SpherePrimitiveComponent = SphereActor.GetComponent<UPrimitiveComponent>();
		SpherePrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
			Renderer.GetDeviceContext(),
			"constants",
			reinterpret_cast<void*>(MVP.M)
		);
		SpherePrimitiveComponent->GetVertexShader()->Bind(Renderer.GetDeviceContext(), "constants");
		SpherePrimitiveComponent->GetPixelShader()->Bind(Renderer.GetDeviceContext());
		SpherePrimitiveComponent->Render(Renderer.GetDeviceContext());

		MVP = MCube * V * P;

		auto CubePrimitiveComponent = CubeActor.GetComponent<UPrimitiveComponent>();
		CubePrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
			Renderer.GetDeviceContext(),
			"constants",
			reinterpret_cast<void*>(MVP.M)
		);
		CubePrimitiveComponent->GetVertexShader()->Bind(Renderer.GetDeviceContext(), "constants");
		CubePrimitiveComponent->GetPixelShader()->Bind(Renderer.GetDeviceContext());
		CubePrimitiveComponent->Render(Renderer.GetDeviceContext());

		VertexShader->UpdateConstantBuffer(
			Renderer.GetDeviceContext(),
			"constants",
			reinterpret_cast<void*>((V * P).M)
		);

		XAxisDrawer.Render(Renderer.GetDeviceContext());
		YAxisDrawer.Render(Renderer.GetDeviceContext());
		ZAxisDrawer.Render(Renderer.GetDeviceContext());

		UI.RenderUI();

		Renderer.SwapBuffer();
	}

	EditorUI.Release();

	return 0;
}