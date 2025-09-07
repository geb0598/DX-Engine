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

// ---------------------------------------------------------- //

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	UWindow Window(1024, 1024, "Hello World!");

	URenderer& Renderer = URenderer::GetInstance(Window.GethWnd());

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)Window.GethWnd());
	ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

	bool bIsExit = false;

	// ----------------------------------------------------------------------------- //
									/* TEST CODE */
	// ----------------------------------------------------------------------------- //

	// Load Scene from JSON File
	LoadScene("Scene/DefaultScene.Scene", Window.GethWnd());

	AActor CameraActor;
	CameraActor.AddComponent<USceneComponent>(&CameraActor, FVector(0.0f, 0.0f, -50.f), FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
	CameraActor.AddComponent<UCameraComponent>(&CameraActor);
	
	// ----------------------------------------------------------------------------- //
	
	float rotator = 0.0f;

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
		
		Renderer.Prepare();

		FMatrix M;
		//FMatrix V = CameraActor.GetComponent<UCameraComponent>()->GetViewMatrix();
		FMatrix V = FMatrix::CreateLookAt(CameraActor.GetComponent<USceneComponent>()->GetLocation(), FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f));
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
				//FMatrix MVP = M * V;
				//FMatrix MVP = M;

				PrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
					Renderer.GetDeviceContext(), 
					"constants", 
					reinterpret_cast<void*>(MVP.M));
				PrimitiveComponent->GetVertexShader()->Bind(Renderer.GetDeviceContext(), "constants");
				PrimitiveComponent->GetPixelShader()->Bind(Renderer.GetDeviceContext());
				PrimitiveComponent->Render(Renderer.GetDeviceContext());
			}
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");

		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		Renderer.SwapBuffer();
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	return 0;
}