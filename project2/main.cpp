// 직접 정의한 헤더 파일
#include "window.hpp"
#include "URenderer.hpp"

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
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowCLass 구조체에 넣는다.
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	// 윈도우 클래스 등록
	RegisterClassW(&wndclass);

	// 1024 x 1024 크기에 윈도우 생성
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

	// Renderer Class를 생성한다.
	URenderer renderer;

	// D3D11 생성하는 함수를 호출한다.
	renderer.Create(hWnd);

	// 렌더러 생성 직후에 쉐이더를 생성하는 함수를 호출한다.
	renderer.CreateShader();

	// 상수 버퍼 생성
	renderer.CreateConstantBuffer();

	// IMGUI 생성
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	// 정점 버퍼를 생성한다.
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

	ID3D11Buffer* vertexBufferTriangle = renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
	
	bool bIsExit = false;

	enum ETypePrimitive
	{
		EPT_Triangle,
		EPT_Cube,
		EPT_Sphere,
		EPT_Max,
	};

	ETypePrimitive typePrimitive = EPT_Triangle;
	// 도형의 움직임 정도를 담을 offset 변수
	FVector offset(0.0f);

	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false)
	{
		MSG msg;

		// 처리할 메시지가 더 이상 없을 때까지 수행
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			// 키 입력 메시지를 번역
			TranslateMessage(&msg);
			// 메시지를 적절한 윈도우 프로시저에 전달, 메시지가 위에서 등록한 WndProc으로 전달됨
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
			else if (msg.message == WM_KEYDOWN)	// 키보드 눌렸을 때
			{
				// 눌린 키가 방향키라면 해당 방향에 맞춰서
				// offset 변수의 x, y 멤버 변수의 값을 조정한다
				if (msg.wParam == VK_LEFT)
				{
					offset.x -= 0.01f;
				}
				if (msg.wParam == VK_RIGHT)
				{
					offset.x += 0.01f;
				}
				if (msg.wParam == VK_UP)
				{
					offset.y += 0.01f;
				}
				if (msg.wParam == VK_DOWN)
				{
					offset.y -= 0.01f;
				}
			}
		}

		// 매번 실행되는 코드를 여기에 추가한다.
		
		// 준비 작업
		renderer.Prepare();
		renderer.PrepareShader();


		// ImGui 렌더링 준비, 컨트롤 설정, 렌더링 요청 등
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render() 사이인 여기에 위치합니다.
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

		// offset을 상수 버퍼로 업데이트한다.
		renderer.UpdateConstant(offset);

		// 아래 Switch를 통해 현재 Primitive Type에 맞춰서 VertexBuffer와 numVertices 변수를 선택한다.
		switch (typePrimitive)
		{
		case EPT_Triangle :
			renderer.RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);
			break;
		case EPT_Cube :
			renderer.RenderPrimitive(vertexBufferCube, numVerticesCube);
			break;
		case EPT_Sphere :
			renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			break;
		}

		// 현재 화면에 보여지는 버퍼와 그리기 작업을 위한 버퍼를 서로 교환한다.
		renderer.SwapBuffer();
	}

	// ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// 버텍스 버퍼 소멸은 Renderer 소멸 전에 처리한다
	renderer.ReleaseVertexBuffer(vertexBufferTriangle);
	renderer.ReleaseVertexBuffer(vertexBufferCube);
	renderer.ReleaseVertexBuffer(vertexBufferSphere);

	// 쉐이더 소멸 직전에 상수 버퍼를 소멸시키는 함수를 호출한다.
	renderer.ReleaseConstantBuffer();
	// 렌더러 소멸 직전에 쉐이더를 소멸시키는 함수를 호출한다.
	renderer.ReleaseShader();
	// D3D11 소멸시키는 함수를 호출한다.
	renderer.Release();

	return 0;
}