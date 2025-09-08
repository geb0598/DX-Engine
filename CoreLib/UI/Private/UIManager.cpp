#include "../Public/UIManager.h"

/* private */

UIManager::UIManager() {}
UIManager::~UIManager() {}

const uint32 UIManager::LogLegionSize = 5;

/* public */

void UIManager::Initialize(HWND HWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)HWnd);
	ImGui_ImplDX11_Init(Device, DeviceContext);
}

void UIManager::RenderControlPanel()
{
	ImGui::SetNextWindowSize(ImVec2(350, 325), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
	if (!ImGui::Begin("Jungle Control Panel", nullptr))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Hello Jungle World!");
	
	/* Show FPS */
	TimeManager& Timer = TimeManager::Instance();

	ImGui::Text("FPS %d (%.2f ms)", (int)Timer.GetFPS(), Timer.GetDeltaTimeInMS());

	/* Choose Primitive */

	// kind
	const char* Items[] = { "Cube", "Sphere", "Triangle" };
	// index
	static int CurrentItem = 0;
	// draw Combo Box
	ImGui::Combo("Primitive", &CurrentItem, Items, IM_ARRAYSIZE(Items));

	/* Number of Spawns */
	static int NumberOfSpawns = 0;
	// draw button and bind event
	if (ImGui::Button("Spawn"))
	{
		// Code to delete or add primive by NumberOfSpawns variable
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	ImGui::DragInt("Number of spawns", &NumberOfSpawns, 0.2f, 0, 20);

	ImGui::Separator();

	/*Scene Control*/

	// draw text input field
	static char SceneName[20] = {};
	ImGui::InputText("Scene name", SceneName, IM_ARRAYSIZE(SceneName), ImGuiInputTextFlags_EnterReturnsTrue);

	// draw buttons
	if (ImGui::Button("New scene"))
	{
		// Save current scene, make new scene and call it
	}
	if (ImGui::Button("Save scene"))
	{

		// Save current scene
	}
	if (ImGui::Button("Load scene"))
	{
		// Save current scene and load other scene
	}

	ImGui::Separator();

	/* Set orthogonal option */

	static bool IsOrthogonal = false;

	ImGui::Checkbox("Orthogonal", &IsOrthogonal);

	/* Set POV */

	static float Fov = 0.0f;

	ImGui::DragFloat("FOV", &Fov, 0.2f, 20.0f, 120.0f);

	/* Set Camera Location */

	static float CameraLocationX = 0.0f;
	static float CameraLocationY = 0.0f;
	static float CameraLocationZ = 0.0f;

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraLocationX", &CameraLocationX, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraLocationY", &CameraLocationY, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraLocationZ", &CameraLocationZ, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Camera Location");

	/* Set Camera Rotation */

	static float CameraRotationX = 0.0f;
	static float CameraRotationY = 0.0f;
	static float CameraRotationZ = 0.0f;

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraRotationX", &CameraRotationX, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraRotationY", &CameraRotationY, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraRotationZ", &CameraRotationZ, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Camera Rotation");

	ImGui::End();
	return;
}

void UIManager::RenderPropertyWindow()
{
	ImGui::SetNextWindowSize(ImVec2(350, 100), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(200, 450), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
	if (!ImGui::Begin("Jungle Property Window", nullptr))
	{
		ImGui::End();
		return;
	}

	/* Set object translation */

	static float ObjectTranslationX = 0.0f;
	static float ObjectTranslationY = 0.0f;
	static float ObjectTranslationZ = 0.0f;

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectTranslationX", &ObjectTranslationX, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectTranslationY", &ObjectTranslationY, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectTranslationZ", &ObjectTranslationZ, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Translation");

	/* Set object rotation */

	static float ObjectRotationX = 0.0f;
	static float ObjectRotationY = 0.0f;
	static float ObjectRotationZ = 0.0f;

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectRotationX", &ObjectRotationX, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectRotationY", &ObjectRotationY, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectRotationZ", &ObjectRotationZ, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Rotation");

	/* Set object translation */

	static float ObjectScaleX = 0.0f;
	static float ObjectScaleY = 0.0f;
	static float ObjectScaleZ = 0.0f;

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectScaleX", &ObjectScaleX, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectScaleY", &ObjectScaleY, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectScaleZ", &ObjectScaleZ, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Scale");

	ImGui::End();
	return;
}

void UIManager::RenderConsole()
{
	ImGui::SetNextWindowSize(ImVec2(800, 275), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(425, 800), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
	if (!ImGui::Begin("Jungle Console", nullptr))
	{
		ImGui::End();
		return;
	}

	ImGui::TextWrapped(
		"This example implements a console with basic coloring, completion (TAB key) and history (Up/Down keys). A more elaborate "
		"implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
	ImGui::TextWrapped("Enter 'HELP' for help.");

	if (ImGui::SmallButton("Add Debug Text"))
	{
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Add Debug Error"))
	{
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear"))
	{
		// erase all log in console
		ClearLog();
	}
	ImGui::SameLine();
	bool CopyToClipBoard = ImGui::SmallButton("Copy");

	ImGui::Separator();

	// Options, Filter
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Tooltip);
	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();

	static char OptionInput[200] = {};

	ImGui::SetNextItemWidth(200);
	ImGui::InputText("Filter (\"incl,-excl\") (\"error\")", OptionInput, sizeof(OptionInput));
	ImGui::Separator();

	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 100), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar))
	{
		// show debug logs here
		for (const FString& Log : Logs)
		{
			ImGui::TextUnformatted(Log.c_str());
		}
	}
	ImGui::EndChild();

	ImGui::Separator();
	
	static char UserInput[200] = {};

	ImGui::SetNextItemWidth(300);
	ImGui::InputText("Input", UserInput, sizeof(UserInput));

	ImGui::End();
	return;
}

void UIManager::RenderUI()
{
	// 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 ImGui::Render() 사이인 여기에 위치합니다.
	// ImGui 렌더링 준비, 컨트롤 설정, 렌더링 요청 등
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//RenderControlPanel();
	//RenderPropertyWindow();
	RenderConsole();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

}

void UIManager::Release()
{
	// ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// static

// Get Instance of the Singleton Object
UIManager& UIManager::Instance()
{
	static UIManager Instance;
	return Instance;
}