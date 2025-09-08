#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_internal.h"

// ---------------------------------------------------------- //
#include "Actor/Actor.h"
#include "Component/Component.h"
#include "Renderer/Renderer.h"
#include "Window/Window.h"

// ---------------------------------------------------------- //
#include <chrono>
#include "Sphere.h"

FVertexSimple triangle_vertices[] =
{
	{  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top vertex (red)
	{  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f }, // Bottom-right vertex (green)
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }  // Bottom-left vertex (blue)
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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)Window.GethWnd());
	ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

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

	AActor Actor;

	TArray<FVertex> VertexArray;
	for (size_t i = 0; i < sizeof(triangle_vertices) / sizeof(FVertexSimple); ++i)
	{
		VertexArray.push_back(static_cast<FVertex>(triangle_vertices[i]));
	}

	std::shared_ptr<UMesh> Mesh = std::make_shared<UMesh>(Renderer.GetDevice(), VertexArray);

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

	Actor.AddComponent<UPrimitiveComponent>(&Actor, Mesh, VertexShader, PixelShader);
	Actor.AddComponent<USceneComponent>(&Actor, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 60.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f));

	// -------------------------- Add Input Component -------------------------------- //
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

		// ------------------------- Temporary Timer ---------------------------------- //
		auto CurrentTime = std::chrono::high_resolution_clock::now();
		auto DeltaTime = LastTime - CurrentTime;
		LastTime = CurrentTime;
		CameraActor.GetComponent<UInputComponent>()->Update(
			std::chrono::duration_cast<std::chrono::duration<double>>(DeltaTime).count()
		);
		/*
		CameraActor.GetComponent<USceneComponent>()->RotateTranform({ 0.0f,
			float(180 * std::chrono::duration_cast<std::chrono::duration<double>>(DeltaTime).count()),
			0.0f });
			*/
		// ---------------------------------------------------------------------------- //

		Renderer.Prepare();

		// ---------------------------------------------------------------------------- //
		FMatrix M = Actor.GetComponent<USceneComponent>()->GetModelingMatrix();
		FMatrix V = CameraActor.GetComponent<UCameraComponent>()->GetViewMatrix();
		FMatrix P = CameraActor.GetComponent<UCameraComponent>()->GetProjectionMatrix(Window.getAspectRatio());

		FMatrix MVP = M * V * P;
		// ---------------------------------------------------------------------------- //

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");

		// Get location and rotation
		auto* SceneComp = CameraActor.GetComponent<USceneComponent>();
		auto Location = SceneComp->GetLocation();
		auto Rotation = SceneComp->GetRotation();

		ImGui::Text("Location | X: %f, Y: %f, Z: %f", Location.X, Location.Y, Location.Z);
		ImGui::Text("Rotation | Pitch(X): %f, Yaw(Y): %f, Roll(Z): %f", Rotation.X, Rotation.Y, Rotation.Z);

		// ------------------ Position sliders ------------------
		static float posX = Location.X;
		static float posY = Location.Y;
		static float posZ = Location.Z;

		bool posChanged = false;
		posChanged |= ImGui::SliderFloat("Pos X", &posX, -1000.0f, 1000.0f);
		posChanged |= ImGui::SliderFloat("Pos Y", &posY, -1000.0f, 1000.0f);
		posChanged |= ImGui::SliderFloat("Pos Z", &posZ, -1000.0f, 1000.0f);

		if (posChanged)
		{
			SceneComp->SetLocation(FVector(posX, posY, posZ));
		}

		// ------------------ Rotation sliders ------------------
		static float rotX = Rotation.X;
		static float rotY = Rotation.Y;
		static float rotZ = Rotation.Z;

		bool rotChanged = false;
		rotChanged |= ImGui::SliderFloat("Pitch (X)", &rotX, -180.0f, 180.0f);
		rotChanged |= ImGui::SliderFloat("Yaw (Y)", &rotY, -180.0f, 180.0f);
		rotChanged |= ImGui::SliderFloat("Roll (Z)", &rotZ, -180.0f, 180.0f);

		if (rotChanged)
		{
			SceneComp->SetRotation({ rotX, rotY, rotZ });
		}

		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		auto PrimitiveComponent = Actor.GetComponent<UPrimitiveComponent>();
		PrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
			Renderer.GetDeviceContext(),
			"constants",
			reinterpret_cast<void*>(MVP.M)
		);
		PrimitiveComponent->GetVertexShader()->Bind(Renderer.GetDeviceContext(), "constants");
		PrimitiveComponent->GetPixelShader()->Bind(Renderer.GetDeviceContext());
		PrimitiveComponent->Render(Renderer.GetDeviceContext());

		Renderer.SwapBuffer();
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}