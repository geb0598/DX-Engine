#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_internal.h"

// ---------------------------------------------------------- //
#include "Actor/Actor.h"
#include "Component/Component.h"
#include "Renderer/Renderer.h"
#include "Window/Window.h"
#include "UI/UI.h"
#include "RayCaster/Raycaster.h"
#include "Axis/Axis.h"

// ---------------------------------------------------------- //

#include "Sphere.h"

FVertexSimple XAxisVertices[] =
{
	{  0.0f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Axis Color(Red)
	{ AXIS_LENGTH, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
};

FVertexSimple YAxisVertices[] =
{
	{  0.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Axis Color(Green)
	{ 0.0f, AXIS_LENGTH, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
};

FVertexSimple ZAxisVertices[] =
{
	{  0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }, // Axis Color(Blue)
	{ 0.0f, 0.0f, AXIS_LENGTH,  0.0f, 0.0f, 1.0f, 1.0f },
};

FVertexSimple triangle_vertices[] =
{
	{  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f },  // Bottom-left vertex (blue)
	{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f } // Bottom-right vertex (green)
};

FVertexSimple cube_vertices[] =
{
	// Front face (Z+)
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f }, // Bottom-left (red)
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f }, // Top-left (yellow)
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right (green)
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f }, // Top-left (yellow)
	{  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f }, // Top-right (blue)
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right (green)

	// Back face (Z-)
	{ -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, 1.0f }, // Bottom-left (cyan)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f }, // Bottom-right (magenta)
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f }, // Top-left (blue)
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f }, // Top-left (blue)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f }, // Bottom-right (magenta)
	{  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f }, // Top-right (yellow)

	// Left face (X-)
	{ -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, 1.0f }, // Bottom-left (purple)
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f }, // Top-left (blue)
	{ -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right (green)
	{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, 1.0f }, // Top-left (blue)
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, 1.0f }, // Top-right (yellow)
	{ -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right (green)

	// Right face (X+)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.5f, 0.0f, 1.0f }, // Bottom-left (orange)
	{  0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, 1.0f }, // Bottom-right (gray)
	{  0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 0.5f, 1.0f }, // Top-left (purple)
	{  0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 0.5f, 1.0f }, // Top-left (purple)
	{  0.5f, -0.5f,  0.5f,  0.5f, 0.5f, 0.5f, 1.0f }, // Bottom-right (gray)
	{  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.5f, 1.0f }, // Top-right (dark blue)

	// Top face (Y+)
	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.5f, 1.0f }, // Bottom-left (light green)
	{ -0.5f,  0.5f,  0.5f,  0.0f, 0.5f, 1.0f, 1.0f }, // Top-left (cyan)
	{  0.5f,  0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f }, // Bottom-right (white)
	{ -0.5f,  0.5f,  0.5f,  0.0f, 0.5f, 1.0f, 1.0f }, // Top-left (cyan)
	{  0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.0f, 1.0f }, // Top-right (brown)
	{  0.5f,  0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f }, // Bottom-right (white)

	// Bottom face (Y-)
	{ -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.0f, 1.0f }, // Bottom-left (brown)
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top-left (red)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f }, // Bottom-right (purple)
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top-left (red)
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f }, // Top-right (green)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f }, // Bottom-right (purple)
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	UWindow Window(1024, 1024, "Hello World!");

	URenderer& Renderer = URenderer::GetInstance(Window.GethWnd());

	UIManager &UI = UIManager::Instance();
	UI.Initialize(Window.GethWnd(), Renderer.GetDevice(), Renderer.GetDeviceContext());

	URayCaster& RayCaster = URayCaster::Instance();

	UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	bool bIsExit = false;

	enum ETypePrimitive
	{
		EPT_Triangle,
		EPT_Cube,
		EPT_Sphere,
		EPT_Max,
	};

	ETypePrimitive typePrimitive = EPT_Triangle;
	FVector offset(0.0f);

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
		
		Renderer.Prepare();

		FMatrix MVP;
		
		MVP = MTriangle * V * P;

		auto PrimitiveComponent = TriangleActor.GetComponent<UPrimitiveComponent>();
		PrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
			Renderer.GetDeviceContext(),
			"constants",
			reinterpret_cast<void*>(MVP.M)
		);
		PrimitiveComponent->GetVertexShader()->Bind(Renderer.GetDeviceContext(), "constants");
		PrimitiveComponent->GetPixelShader()->Bind(Renderer.GetDeviceContext());
		PrimitiveComponent->Render(Renderer.GetDeviceContext());

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

	UI.Release();

	return 0;
}