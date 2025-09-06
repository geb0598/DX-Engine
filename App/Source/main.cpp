// ���� ������ ��� ����
#include "Window/Window.h"
#include "Renderer/Renderer.h"

#include "Sphere.h"

// ---------------------------------------------------------- //
#include "Actor/Actor.h"
#include "Component/Component.h"
#include <vector>


// ---------------------------------------------------------- //

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
	WCHAR WindowClass[] = L"JungleWindowClass";

	WCHAR Title[] = L"Game Tech Lab";

	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	RegisterClassW(&wndclass);

	HWND hWnd = CreateWindowExW(
								0,
								WindowClass,
								Title,
								WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								1024,
								1024,
								nullptr,
								nullptr,
								hInstance,
								nullptr
								);

	URenderer renderer;

	renderer.Create(hWnd);

	// renderer.CreateShader();

	// renderer.CreateConstantBuffer();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	float scaleMod = 0.1f;

	for (UINT i = 0; i < numVerticesSphere; ++i)
	{
		sphere_vertices[i].x *= scaleMod;
		sphere_vertices[i].y *= scaleMod;
		sphere_vertices[i].z *= scaleMod;
	}

	// ID3D11Buffer* vertexBufferTriangle = renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	// ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	// ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
	
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

	auto Convert = [](FVertexSimple VertexSimple) {
		FVertex Vertex;
		Vertex.Position.X = VertexSimple.x;
		Vertex.Position.Y = VertexSimple.y;
		Vertex.Position.Z = VertexSimple.z;
		Vertex.Color.X = VertexSimple.r;
		Vertex.Color.Y = VertexSimple.g;
		Vertex.Color.Z = VertexSimple.b;
		Vertex.Color.W = VertexSimple.a;
		return Vertex;
	};

	TArray<FVertex> Vertices;
	for (size_t i = 0; i < sizeof(triangle_vertices) / sizeof(FVertexSimple); ++i)
	{
		Vertices.push_back(Convert(triangle_vertices[i]));
	}

	std::shared_ptr<UMesh> Mesh = std::make_shared<UMesh>(renderer.Device, Vertices);
	TArray<D3D11_INPUT_ELEMENT_DESC> InputElem(std::begin(FVertex::InputLayout), std::end(FVertex::InputLayout));
	std::shared_ptr<UVertexShader> VertexShader = std::make_shared<UVertexShader>(
		renderer.Device, 
		"./Shader/ShaderW0_VS.hlsl", 
		"mainVS",
		InputElem
	);
	std::shared_ptr<UPixelShader> PixelShader = std::make_shared<UPixelShader>(
		renderer.Device,
		"./Shader/ShaderW0_PS.hlsl",
		"mainPS"
	);
	Actor.AddComponent<UPrimitiveComponent>(&Actor, Mesh, VertexShader, PixelShader);
	// Actor.AddComponent<UPrimitiveComponent>(Actor);

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

		
		renderer.Prepare();
		// renderer.PrepareShader();


		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");

		if (ImGui::Button("Change primitive"))
		{
			switch (typePrimitive)
			{
			case EPT_Triangle:
				typePrimitive = EPT_Cube;
				break;
			case EPT_Cube:
				typePrimitive = EPT_Sphere;
				break;
			case EPT_Sphere:
				typePrimitive = EPT_Triangle;
				break;
			}
		}

		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// renderer.UpdateConstant(offset);

		switch (typePrimitive)
		{
		case EPT_Triangle :
			// renderer.RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);
			break;
		case EPT_Cube :
			// renderer.RenderPrimitive(vertexBufferCube, numVerticesCube);
			break;
		case EPT_Sphere :
			// renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			break;
		}

		auto PrimitiveComponent = Actor.GetComponent<UPrimitiveComponent>();
		PrimitiveComponent->GetVertexShader()->UpdateConstantBuffer(
			renderer.DeviceContext,
			"constants",
			reinterpret_cast<void*>(&offset)
		);
		PrimitiveComponent->Render(renderer.DeviceContext);

		renderer.SwapBuffer();
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// renderer.ReleaseVertexBuffer(vertexBufferTriangle);
	// renderer.ReleaseVertexBuffer(vertexBufferCube);
	// renderer.ReleaseVertexBuffer(vertexBufferSphere);

	// renderer.ReleaseConstantBuffer();
	// renderer.ReleaseShader();
	renderer.Release();

	return 0;
}