#include <random>

#include "UI/Public/UIManager.h"
#include "TIme/Time.h"
#include "Scene/Scene.h"
#include "Utilities/Public/Logger.h"
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

	// 초록색 테마 설정
	SetGreenTheme();
}

void UIManager::RenderControlPanel()
{
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
	static int NumberOfSpawns = 1;
	// draw button and bind event
	if (ImGui::Button("Spawn"))
	{
		auto PrimitiveType = UPrimitiveComponent::EType::Triangle;
		// Code to delete or add primive by NumberOfSpawns variable
		if (Items[CurrentItem] == "Cube")
		{
			PrimitiveType = UPrimitiveComponent::EType::Cube;
		}
		else if (Items[CurrentItem] == "Sphere")
		{
			PrimitiveType = UPrimitiveComponent::EType::Sphere;
		}
		else if (Items[CurrentItem] == "Triangle")
		{
			PrimitiveType = UPrimitiveComponent::EType::Triangle;
		}

		std::random_device RandomDevice;
		std::mt19937 Generator(RandomDevice());
		std::uniform_real_distribution<float> UniformDist(-10.0f, 10.0f);

		for (int i = 0; i < NumberOfSpawns; ++i)
		{
			float X = UniformDist(Generator);
			float Y = UniformDist(Generator);
			float Z = UniformDist(Generator);

			auto& SceneManager = USceneManager::GetInstance();
			AActor* NewActor = SceneManager.CreateActorFromPrimitive(
				{	 X,    Y,    Z },
				{ 0.0f, 0.0f, 0.0f },
				{ 1.0f, 1.0f, 1.0f },
				PrimitiveType
			);
			SceneManager.AddActorToCurrentScene(NewActor);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	ImGui::DragInt("Number of spawns", &NumberOfSpawns, 0.2f, 0, 20);

	ImGui::Separator();

	/*Scene Control*/

	// draw text input field
	static char SceneName[20] = {};
	ImGui::InputText("Scene name", SceneName, IM_ARRAYSIZE(SceneName), ImGuiInputTextFlags_EnterReturnsTrue);

	FString SceneNameStr = "Scene/" + FString(SceneName) + ".Scene";
	if (SceneNameStr == "Scene/.Scene")
	{
		SceneNameStr = "Scene/Default.Scene";
	}

	// draw buttons
	if (ImGui::Button("New scene"))
	{
		USceneManager::GetInstance().NewScene(SceneNameStr);
	}
	if (ImGui::Button("Save scene"))
	{
		USceneManager::GetInstance().SaveScene(SceneNameStr);
	}
	if (ImGui::Button("Load scene"))
	{
		USceneManager::GetInstance().LoadScene(SceneNameStr);
	}

	ImGui::Separator();

	// ------------------------------- Camera UI ------------------------------------- //
	auto CameraActor = USceneManager::GetInstance().GetMainCameraActor();
	auto CameraComponent = CameraActor->GetComponent<UCameraComponent>();
	auto CameraSceneComponent = CameraActor->GetComponent<USceneComponent>();


	/* Set orthogonal option */

	bool IsOrthogonal = CameraComponent->IsOrthogonal();

	// TODO: Camera requires SetOrthogonal method
	ImGui::Checkbox("Orthogonal", &IsOrthogonal);

	if (IsOrthogonal)
	{
		CameraComponent->EnableOrthogonal();
	}
	else
	{
		CameraComponent->DisableOrthogonal();
	}

	/* Set POV */

	float Fov = CameraComponent->GetFieldOfView();
	if (ImGui::DragFloat("FOV", &Fov, 0.2f, 20.0f, 120.0f))
	{
		// NOTE: Field Of View is managed with Radian
		CameraComponent->SetFieldOfView(Fov);
	}


	/* Set Camera Location */

	auto CameraLocation = CameraSceneComponent->GetLocation();

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraLocationX", &CameraLocation.X, 0.05f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraLocationY", &CameraLocation.Y, 0.05f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraLocationZ", &CameraLocation.Z, 0.05f);
	ImGui::SameLine();
	ImGui::Text("Camera Location");

	CameraSceneComponent->SetLocation(CameraLocation);

	/* Set Camera Rotation */

	auto CameraRotation = CameraSceneComponent->GetRotation();

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraRotationX", &CameraRotation.X, 0.05f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraRotationY", &CameraRotation.Y, 0.05f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##CameraRotationZ", &CameraRotation.Z, 0.05f);
	ImGui::SameLine();
	ImGui::Text("Camera Rotation");

	CameraSceneComponent->SetRotation(CameraRotation);
	ImGui::End();
	return;
}

void UIManager::RenderPropertyWindow()
{
	if (!ImGui::Begin("Jungle Property Window", nullptr))
	{
		ImGui::End();
		return;
	}

	/* Set object translation */

	auto CurrentActor = USceneManager::GetInstance().GetCurrentActor();
	if (!CurrentActor)
	{
		ImGui::End();
		return;
	}

	auto CurrentActorSceneComponent = CurrentActor->GetComponent<USceneComponent>();

	auto ObjectLocation = CurrentActorSceneComponent->GetLocation();

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectTranslationX", &ObjectLocation.X, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectTranslationY", &ObjectLocation.Y, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectTranslationZ", &ObjectLocation.Z, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Translation");

	CurrentActorSceneComponent->SetLocation(ObjectLocation);

	/* Set object rotation */

	auto ObjectRotation = CurrentActorSceneComponent->GetRotation();

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectRotationX", &ObjectRotation.X, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectRotationY", &ObjectRotation.Y, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectRotationZ", &ObjectRotation.Z, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Rotation");

	CurrentActorSceneComponent->SetRotation(ObjectRotation);

	/* Set object translation */

	auto ObjectScale = CurrentActorSceneComponent->GetScale();

	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectScaleX", &ObjectScale.X, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectScaleY", &ObjectScale.Y, 0.2f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##ObjectScaleZ", &ObjectScale.Z, 0.2f);
	ImGui::SameLine();
	ImGui::Text("Scale");

	CurrentActorSceneComponent->SetScale(ObjectScale);

	ImGui::End();
	return;
}

void UIManager::RenderConsole()
{
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
		UE_LOG(Info, "Test info message %d", rand() % 1000);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Add Debug Error"))
	{
		UE_LOG(Error, "Test error message %d", rand() % 1000);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear"))
	{
		CConsoleOutput::GetConsoleMessages().clear();
	}
	ImGui::SameLine();
	bool CopyToClipBoard = ImGui::SmallButton("Copy");

	ImGui::Separator();

	// 로그 레벨 필터 체크박스들
	static bool showDebug = true;
	static bool showInfo = true;
	static bool showWarning = true;
	static bool showError = true;
	static bool showFatal = true;

	ImGui::Text("Log Levels:");
	ImGui::SameLine();
	ImGui::Checkbox("DEBUG", &showDebug);
	ImGui::SameLine();
	ImGui::Checkbox("INFO", &showInfo);
	ImGui::SameLine();
	ImGui::Checkbox("WARNING", &showWarning);
	ImGui::SameLine();
	ImGui::Checkbox("ERROR", &showError);
	ImGui::SameLine();
	ImGui::Checkbox("FATAL", &showFatal);

	// Search
	static char searchText[256] = {};
	ImGui::SetNextItemWidth(300);
	ImGui::InputTextWithHint("##Search", "Search messages...", searchText, sizeof(searchText));
	ImGui::SameLine();
	if (ImGui::Button("Clear Search")) {
		searchText[0] = '\0';
	}

	ImGui::Separator();

	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 100), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar))
	{
		{
			const auto& messages = CConsoleOutput::GetConsoleMessages();

			FString searchString = searchText;
			std::transform(searchString.begin(), searchString.end(), searchString.begin(), ::tolower);

			for (const auto& consoleMsg : messages) {
				// Log Level Filtering
				bool shouldShow = false;
				switch (consoleMsg.level) {
				case ELogLevel::DebugLevel:   shouldShow = showDebug; break;
				case ELogLevel::InfoLevel:    shouldShow = showInfo; break;
				case ELogLevel::WarningLevel: shouldShow = showWarning; break;
				case ELogLevel::ErrorLevel:   shouldShow = showError; break;
				case ELogLevel::FatalLevel:   shouldShow = showFatal; break;
				}

				if (!shouldShow) continue;

				// Serach Filtering
				if (searchString.length() > 0) {
					FString messageLower = consoleMsg.message;
					std::transform(messageLower.begin(), messageLower.end(), messageLower.begin(), ::tolower);
					if (messageLower.find(searchString) == FString::npos) {
						continue;
					}
				}

				// Remove \n at the end if exists
				FString displayMessage = consoleMsg.message;
				if (!displayMessage.empty() && displayMessage.back() == '\n') {
					displayMessage.pop_back();
				}

				// Color coding based on log level
				ImVec4 color;
				switch (consoleMsg.level) {
				case ELogLevel::DebugLevel:   color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break; // 회색
				case ELogLevel::InfoLevel:    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; // 흰색
				case ELogLevel::WarningLevel: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // 노란색
				case ELogLevel::ErrorLevel:   color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // 빨간색
				case ELogLevel::FatalLevel:   color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break; // 진한 빨간색
				}

				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::TextUnformatted(displayMessage.c_str());
				ImGui::PopStyleColor();
			}

			// Auto-scroll to bottom
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
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
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	RenderControlPanel();
	RenderPropertyWindow();
	RenderConsole();
	RenderStatWindow();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

}

void UIManager::RenderStatWindow()
{
	if (!ImGui::Begin("Memory Statistics:", nullptr))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Memory Statistics:");
	ImGui::Text("Total Allocation Count: %u", FMemory::TotalAllocationCount);
	ImGui::Text("Total Allocation Bytes: %u bytes", FMemory::TotalAllocationBytes);

	ImGui::End();
}

void UIManager::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void UIManager::SetGreenTheme()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	ImVec4 darkGreen = ImVec4(0.15f, 0.25f, 0.18f, 1.0f);      // 진한 초록 배경
	ImVec4 mediumGreen = ImVec4(0.20f, 0.35f, 0.25f, 1.0f);    // 중간 초록
	ImVec4 lightGreen = ImVec4(0.30f, 0.55f, 0.40f, 1.0f);     // 밝은 초록
	ImVec4 accentGreen = ImVec4(0.40f, 0.70f, 0.50f, 1.0f);    // 강조 초록
	ImVec4 brightGreen = ImVec4(0.50f, 0.85f, 0.60f, 1.0f);    // 매우 밝은 초록

	// 텍스트 색상
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.95f, 0.92f, 1.0f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.70f, 0.65f, 1.0f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 0.35f);

	// 윈도우 배경
	colors[ImGuiCol_WindowBg] = ImVec4(darkGreen.x, darkGreen.y, darkGreen.z, 0.95f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.22f, 0.16f, 0.50f);
	colors[ImGuiCol_PopupBg] = ImVec4(darkGreen.x, darkGreen.y, darkGreen.z, 0.95f);

	// 테두리
	colors[ImGuiCol_Border] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	// 프레임 (입력 필드, 슬라이더 등)
	colors[ImGuiCol_FrameBg] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 0.80f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 0.80f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 0.80f);

	// 타이틀바
	colors[ImGuiCol_TitleBg] = ImVec4(darkGreen.x - 0.05f, darkGreen.y - 0.05f, darkGreen.z - 0.05f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(darkGreen.x, darkGreen.y, darkGreen.z, 0.75f);

	// 메뉴바
	colors[ImGuiCol_MenuBarBg] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 1.0f);

	// 스크롤바
	colors[ImGuiCol_ScrollbarBg] = ImVec4(darkGreen.x, darkGreen.y, darkGreen.z, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 1.0f);

	// 체크박스
	colors[ImGuiCol_CheckMark] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 1.0f);

	// 슬라이더
	colors[ImGuiCol_SliderGrab] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 1.0f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);

	// 버튼
	colors[ImGuiCol_Button] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 0.80f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);

	// 헤더 (트리 노드, 컬럼 헤더 등)
	colors[ImGuiCol_Header] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 0.76f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 0.86f);
	colors[ImGuiCol_HeaderActive] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);

	// 구분자
	colors[ImGuiCol_Separator] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 1.0f);

	// 크기 조절 그립
	colors[ImGuiCol_ResizeGrip] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 0.95f);

	// 탭
	colors[ImGuiCol_Tab] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 0.86f);
	colors[ImGuiCol_TabHovered] = ImVec4(lightGreen.x, lightGreen.y, lightGreen.z, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(darkGreen.x + 0.05f, darkGreen.y + 0.05f, darkGreen.z + 0.05f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(mediumGreen.x, mediumGreen.y, mediumGreen.z, 1.0f);

	// 플롯
	colors[ImGuiCol_PlotLines] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 1.0f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 1.0f);

	// 기타
	colors[ImGuiCol_DragDropTarget] = ImVec4(brightGreen.x, brightGreen.y, brightGreen.z, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(accentGreen.x, accentGreen.y, accentGreen.z, 1.0f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	// 스타일 설정
	style.WindowRounding = 5.0f;
	style.ChildRounding = 4.0f;
	style.FrameRounding = 3.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 4.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;
}

// static

// Get Instance of the Singleton Object
UIManager& UIManager::Instance()
{
	static UIManager Instance;
	return Instance;
}