#include "pch.h"
#include "Render/UI/Widget/Public/SkeletalMeshViewerToolbarWidget.h"
#include "Render/UI/Window/Public/SkeletalMeshViewerWindow.h"
#include "ImGui/imgui_internal.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Texture/Public/Texture.h"

IMPLEMENT_CLASS(USkeletalMeshViewerToolbarWidget, UWidget)

/**
 * @brief SkeletalMeshViewerToolbarWidget 생성자
 */
USkeletalMeshViewerToolbarWidget::USkeletalMeshViewerToolbarWidget()
{
}

/**
 * @brief 소멸자
 */
USkeletalMeshViewerToolbarWidget::~USkeletalMeshViewerToolbarWidget()
{
}

/**
 * @brief 초기화
 */
void USkeletalMeshViewerToolbarWidget::Initialize()
{
}

/**
 * @brief 업데이트
 */
void USkeletalMeshViewerToolbarWidget::Update()
{
	// QWER 키보드 단축키 처리 (뷰어 윈도우가 포커스되어 있을 때)
	// 우클릭이 눌러져 있지 않을 때만 동작 (카메라 이동과 충돌 방지)
	if (ImGui::IsWindowFocused() && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Q))
		{
			bSelectModeActive = true;
			CurrentGizmoMode = EGizmoMode::Translate; // Select 모드 활성화 시 기본 변환 모드
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_W))
		{
			bSelectModeActive = false;
			CurrentGizmoMode = EGizmoMode::Translate;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_E))
		{
			bSelectModeActive = false;
			CurrentGizmoMode = EGizmoMode::Rotate;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_R))
		{
			bSelectModeActive = false;
			CurrentGizmoMode = EGizmoMode::Scale;
		}
	}
}

/**
 * @brief 뷰 아이콘 로드
 */
void USkeletalMeshViewerToolbarWidget::LoadViewIcons()
{
	if (bIconsLoaded)
	{
		return;
	}

	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UPathManager& PathManager = UPathManager::GetInstance();

	// 아이콘 경로 (에디터 아이콘 폴더)
	FString IconBasePath = PathManager.GetAssetPath().string() + "\\Icon\\";

	// 각 뷰 타입별 아이콘 로드
	IconPerspective = AssetManager.LoadTexture((IconBasePath + "ViewPerspective.png").data());
	IconTop = AssetManager.LoadTexture((IconBasePath + "ViewTop.png").data());
	IconBottom = AssetManager.LoadTexture((IconBasePath + "ViewBottom.png").data());
	IconLeft = AssetManager.LoadTexture((IconBasePath + "ViewLeft.png").data());
	IconRight = AssetManager.LoadTexture((IconBasePath + "ViewRight.png").data());
	IconFront = AssetManager.LoadTexture((IconBasePath + "ViewFront.png").data());
	IconBack = AssetManager.LoadTexture((IconBasePath + "ViewBack.png").data());

	// Gizmo Mode 아이콘 로드
	IconSelect = AssetManager.LoadTexture((IconBasePath + "Select.png").data());
	IconTranslate = AssetManager.LoadTexture((IconBasePath + "Translate.png").data());
	IconRotate = AssetManager.LoadTexture((IconBasePath + "Rotate.png").data());
	IconScale = AssetManager.LoadTexture((IconBasePath + "Scale.png").data());

	// Other 아이콘 로드
	IconLitCube = AssetManager.LoadTexture((IconBasePath + "LitCube.png").data());
	IconCamera = AssetManager.LoadTexture((IconBasePath + "Camera.png").data());

	// World/Local Space 아이콘 로드
	IconWorldSpace = AssetManager.LoadTexture((IconBasePath + "WorldSpace.png").data());
	IconLocalSpace = AssetManager.LoadTexture((IconBasePath + "LocalSpace.png").data());

	// Snap 아이콘 로드
	IconSnapScale = AssetManager.LoadTexture((IconBasePath + "SnapScale.png").data());

	bIconsLoaded = true;
}

// ========================================
// Viewport MenuBar Helper Functions
// ========================================

bool USkeletalMeshViewerToolbarWidget::DrawIconButton(const char* ID, UTexture* Icon, bool bActive, const char* Tooltip, float ButtonSize, float IconSize)
{
	if (!Icon || !Icon->GetTextureSRV())
	{
		return false;
	}

	ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton(ID, ImVec2(ButtonSize, ButtonSize));
	bool bClicked = ImGui::IsItemClicked();
	bool bHovered = ImGui::IsItemHovered();

	ImDrawList* DL = ImGui::GetWindowDrawList();

	// 배경색
	ImU32 BgColor = bActive ? IM_COL32(20, 20, 20, 255) : (bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255));
	if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

	// 배경 렌더링
	DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + ButtonSize, ButtonPos.y + ButtonSize), BgColor, 4.0f);

	// 테두리
	ImU32 BorderColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(96, 96, 96, 255);
	DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + ButtonSize, ButtonPos.y + ButtonSize), BorderColor, 4.0f);

	// 아이콘 (활성화 시 파란색 틴트)
	ImVec2 IconPos = ImVec2(ButtonPos.x + (ButtonSize - IconSize) * 0.5f, ButtonPos.y + (ButtonSize - IconSize) * 0.5f);
	ImU32 TintColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(255, 255, 255, 255);
	DL->AddImage(Icon->GetTextureSRV(), IconPos, ImVec2(IconPos.x + IconSize, IconPos.y + IconSize), ImVec2(0, 0), ImVec2(1, 1), TintColor);

	// 툴팁
	if (bHovered && Tooltip)
	{
		ImGui::SetTooltip("%s", Tooltip);
	}

	return bClicked;
}

void USkeletalMeshViewerToolbarWidget::RenderGizmoModeButtons()
{
	constexpr float GizmoButtonSize = 24.0f;
	constexpr float GizmoIconSize = 16.0f;
	constexpr float GizmoButtonSpacing = 4.0f;

	// Select 버튼 (Q)
	if (DrawIconButton("##GizmoSelect", IconSelect, bSelectModeActive, "Select (Q)", GizmoButtonSize, GizmoIconSize))
	{
		bSelectModeActive = true;
	}

	ImGui::SameLine(0.0f, GizmoButtonSpacing);

	// Translate 버튼 (W)
	bool bTranslateActive = (CurrentGizmoMode == EGizmoMode::Translate && !bSelectModeActive);
	if (DrawIconButton("##GizmoTranslate", IconTranslate, bTranslateActive, "Translate (W)", GizmoButtonSize, GizmoIconSize))
	{
		CurrentGizmoMode = EGizmoMode::Translate;
		bSelectModeActive = false;
	}

	ImGui::SameLine(0.0f, GizmoButtonSpacing);

	// Rotate 버튼 (E)
	bool bRotateActive = (CurrentGizmoMode == EGizmoMode::Rotate && !bSelectModeActive);
	if (DrawIconButton("##GizmoRotate", IconRotate, bRotateActive, "Rotate (E)", GizmoButtonSize, GizmoIconSize))
	{
		CurrentGizmoMode = EGizmoMode::Rotate;
		bSelectModeActive = false;
	}

	ImGui::SameLine(0.0f, GizmoButtonSpacing);

	// Scale 버튼 (R)
	bool bScaleActive = (CurrentGizmoMode == EGizmoMode::Scale && !bSelectModeActive);
	if (DrawIconButton("##GizmoScale", IconScale, bScaleActive, "Scale (R)", GizmoButtonSize, GizmoIconSize))
	{
		CurrentGizmoMode = EGizmoMode::Scale;
		bSelectModeActive = false;
	}
}

void USkeletalMeshViewerToolbarWidget::RenderRotationSnapControls()
{
	ImGui::SameLine(0.0f, 8.0f);

	// 회전 스냅 토글 버튼
	{
		constexpr float SnapToggleButtonSize = 24.0f;
		constexpr float SnapToggleIconSize = 16.0f;

		ImVec2 SnapToggleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##RotationSnapToggle", ImVec2(SnapToggleButtonSize, SnapToggleButtonSize));
		bool bSnapToggleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapToggleHovered = ImGui::IsItemHovered();

		ImDrawList* SnapToggleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapToggleBgColor;
		if (bRotationSnapEnabled)
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(40, 40, 40, 255) : IM_COL32(20, 20, 20, 255);
		}
		else
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		}
		if (ImGui::IsItemActive())
		{
			SnapToggleBgColor = IM_COL32(50, 50, 50, 255);
		}
		SnapToggleDrawList->AddRectFilled(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBgColor, 4.0f);

		// 테두리
		ImU32 SnapToggleBorderColor = bRotationSnapEnabled ? IM_COL32(150, 150, 150, 255) : IM_COL32(96, 96, 96, 255);
		SnapToggleDrawList->AddRect(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBorderColor, 4.0f);

		// 회전 아이콘 (중앙 정렬)
		ImVec2 SnapToggleIconCenter = ImVec2(
			SnapToggleButtonPos.x + SnapToggleButtonSize * 0.5f,
			SnapToggleButtonPos.y + SnapToggleButtonSize * 0.5f
		);
		float SnapToggleIconRadius = SnapToggleIconSize * 0.4f;
		ImU32 SnapToggleIconColor = bRotationSnapEnabled ? IM_COL32(46, 163, 255, 255) : IM_COL32(220, 220, 220, 255);
		SnapToggleDrawList->AddCircle(SnapToggleIconCenter, SnapToggleIconRadius, SnapToggleIconColor, 12, 1.5f);
		SnapToggleDrawList->PathArcTo(SnapToggleIconCenter, SnapToggleIconRadius + 2.0f, 0.0f, 1.5f, 8);
		SnapToggleDrawList->PathStroke(SnapToggleIconColor, 0, 1.5f);

		if (bSnapToggleClicked)
		{
			bRotationSnapEnabled = !bRotationSnapEnabled;
		}

		if (bSnapToggleHovered)
		{
			ImGui::SetTooltip("Toggle rotation snap");
		}
	}

	ImGui::SameLine(0.0f, 4.0f);

	// 회전 스냅 각도 선택 버튼
	{
		char SnapAngleText[16];
		(void)snprintf(SnapAngleText, sizeof(SnapAngleText), "%.0f°", RotationSnapAngle);

		constexpr float SnapAngleButtonHeight = 24.0f;
		constexpr float SnapAnglePadding = 8.0f;
		const ImVec2 SnapAngleTextSize = ImGui::CalcTextSize(SnapAngleText);
		const float SnapAngleButtonWidth = SnapAngleTextSize.x + SnapAnglePadding * 2;

		ImVec2 SnapAngleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##RotationSnapAngle", ImVec2(SnapAngleButtonWidth, SnapAngleButtonHeight));
		bool bSnapAngleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapAngleHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* SnapAngleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapAngleBgColor = bSnapAngleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			SnapAngleBgColor = IM_COL32(38, 38, 38, 255);
		}
		SnapAngleDrawList->AddRectFilled(SnapAngleButtonPos, ImVec2(SnapAngleButtonPos.x + SnapAngleButtonWidth, SnapAngleButtonPos.y + SnapAngleButtonHeight), SnapAngleBgColor, 4.0f);
		SnapAngleDrawList->AddRect(SnapAngleButtonPos, ImVec2(SnapAngleButtonPos.x + SnapAngleButtonWidth, SnapAngleButtonPos.y + SnapAngleButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 텍스트 그리기
		ImVec2 SnapAngleTextPos = ImVec2(
			SnapAngleButtonPos.x + SnapAnglePadding,
			SnapAngleButtonPos.y + (SnapAngleButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
		);
		SnapAngleDrawList->AddText(SnapAngleTextPos, IM_COL32(220, 220, 220, 255), SnapAngleText);

		if (bSnapAngleClicked)
		{
			ImGui::OpenPopup("##RotationSnapAnglePopup");
		}

		// 각도 선택 팝업
		if (ImGui::BeginPopup("##RotationSnapAnglePopup"))
		{
			ImGui::Text("Rotation Snap Angle");
			ImGui::Separator();

			constexpr float SnapAngles[] = { 5.0f, 10.0f, 15.0f, 22.5f, 30.0f, 45.0f, 60.0f, 90.0f };
			constexpr const char* SnapAngleLabels[] = { "5°", "10°", "15°", "22.5°", "30°", "45°", "60°", "90°" };

			for (int i = 0; i < IM_ARRAYSIZE(SnapAngles); ++i)
			{
				const bool bIsSelected = (std::abs(RotationSnapAngle - SnapAngles[i]) < 0.1f);
				if (ImGui::MenuItem(SnapAngleLabels[i], nullptr, bIsSelected))
				{
					RotationSnapAngle = SnapAngles[i];
				}
			}

			ImGui::EndPopup();
		}

		if (bSnapAngleHovered)
		{
			ImGui::SetTooltip("Choose rotation snap angle");
		}
	}
}

void USkeletalMeshViewerToolbarWidget::RenderScaleSnapControls()
{
	ImGui::SameLine(0.0f, 8.0f);

	// 스케일 스냅 토글 버튼 (아이콘)
	if (IconSnapScale && IconSnapScale->GetTextureSRV())
	{
		constexpr float SnapToggleButtonSize = 24.0f;
		constexpr float SnapToggleIconSize = 16.0f;

		ImVec2 SnapToggleButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ScaleSnapToggle", ImVec2(SnapToggleButtonSize, SnapToggleButtonSize));
		bool bSnapToggleClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapToggleHovered = ImGui::IsItemHovered();

		ImDrawList* SnapToggleDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapToggleBgColor;
		if (bScaleSnapEnabled)
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(40, 40, 40, 255) : IM_COL32(20, 20, 20, 255);
		}
		else
		{
			SnapToggleBgColor = bSnapToggleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		}
		if (ImGui::IsItemActive())
		{
			SnapToggleBgColor = IM_COL32(50, 50, 50, 255);
		}
		SnapToggleDrawList->AddRectFilled(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBgColor, 4.0f);

		// 테두리
		ImU32 SnapToggleBorderColor = bScaleSnapEnabled ? IM_COL32(150, 150, 150, 255) : IM_COL32(96, 96, 96, 255);
		SnapToggleDrawList->AddRect(SnapToggleButtonPos, ImVec2(SnapToggleButtonPos.x + SnapToggleButtonSize, SnapToggleButtonPos.y + SnapToggleButtonSize), SnapToggleBorderColor, 4.0f);

		// 아이콘 렌더링 (중앙 정렬)
		ImVec2 IconMin = ImVec2(
			SnapToggleButtonPos.x + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f,
			SnapToggleButtonPos.y + (SnapToggleButtonSize - SnapToggleIconSize) * 0.5f
		);
		ImVec2 IconMax = ImVec2(IconMin.x + SnapToggleIconSize, IconMin.y + SnapToggleIconSize);
		ImU32 IconTintColor = bScaleSnapEnabled ? IM_COL32(46, 163, 255, 255) : IM_COL32(220, 220, 220, 255);
		SnapToggleDrawList->AddImage((void*)IconSnapScale->GetTextureSRV(), IconMin, IconMax, ImVec2(0, 0), ImVec2(1, 1), IconTintColor);

		if (bSnapToggleClicked)
		{
			bScaleSnapEnabled = !bScaleSnapEnabled;
		}

		if (bSnapToggleHovered)
		{
			ImGui::SetTooltip("Toggle scale snap");
		}
	}

	ImGui::SameLine(0.0f, 4.0f);

	// 스케일 스냅 값 선택 버튼
	{
		char SnapValueText[16];
		(void)snprintf(SnapValueText, sizeof(SnapValueText), "%.4g", ScaleSnapValue);

		constexpr float SnapValueButtonHeight = 24.0f;
		constexpr float SnapValuePadding = 8.0f;
		const ImVec2 SnapValueTextSize = ImGui::CalcTextSize(SnapValueText);
		const float SnapValueButtonWidth = SnapValueTextSize.x + SnapValuePadding * 2;

		ImVec2 SnapValueButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ScaleSnapValue", ImVec2(SnapValueButtonWidth, SnapValueButtonHeight));
		bool bSnapValueClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
		bool bSnapValueHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* SnapValueDrawList = ImGui::GetWindowDrawList();
		ImU32 SnapValueBgColor = bSnapValueHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			SnapValueBgColor = IM_COL32(38, 38, 38, 255);
		}
		SnapValueDrawList->AddRectFilled(SnapValueButtonPos, ImVec2(SnapValueButtonPos.x + SnapValueButtonWidth, SnapValueButtonPos.y + SnapValueButtonHeight), SnapValueBgColor, 4.0f);
		SnapValueDrawList->AddRect(SnapValueButtonPos, ImVec2(SnapValueButtonPos.x + SnapValueButtonWidth, SnapValueButtonPos.y + SnapValueButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 텍스트 그리기
		ImVec2 SnapValueTextPos = ImVec2(
			SnapValueButtonPos.x + SnapValuePadding,
			SnapValueButtonPos.y + (SnapValueButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
		);
		SnapValueDrawList->AddText(SnapValueTextPos, IM_COL32(220, 220, 220, 255), SnapValueText);

		if (bSnapValueClicked)
		{
			ImGui::OpenPopup("##ScaleSnapValuePopup");
		}

		// 값 선택 팝업
		if (ImGui::BeginPopup("##ScaleSnapValuePopup"))
		{
			ImGui::Text("Scale Snap Value");
			ImGui::Separator();

			constexpr float SnapValues[] = { 10.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.1f, 0.0625f, 0.03125f };
			constexpr const char* SnapValueLabels[] = { "10", "1", "0.5", "0.25", "0.125", "0.1", "0.0625", "0.03125" };

			for (int i = 0; i < IM_ARRAYSIZE(SnapValues); ++i)
			{
				const bool bIsSelected = (std::abs(ScaleSnapValue - SnapValues[i]) < 0.0001f);
				if (ImGui::MenuItem(SnapValueLabels[i], nullptr, bIsSelected))
				{
					ScaleSnapValue = SnapValues[i];
				}
			}

			ImGui::EndPopup();
		}

		if (bSnapValueHovered)
		{
			ImGui::SetTooltip("Choose scale snap value");
		}
	}
}

/**
 * @brief 뷰포트 툴바 렌더링 (메인 에디터 뷰포트와 동일한 디자인)
 * 좌측: Gizmo Mode 아이콘 (비활성화) + Rotation Snap (비활성화)
 * 우측: ViewType + Camera Settings + View Mode
 */
void USkeletalMeshViewerToolbarWidget::RenderWidget()
{
	if (!ViewportClient)
	{
		return;
	}

	// 아이콘 로드 (처음 한 번만)
	LoadViewIcons();

	UCamera* Camera = ViewportClient->GetCamera();
	if (!Camera)
	{
		return;
	}

	// 툴바는 메뉴바 영역 위에 별도로 렌더링 (ImGui::BeginMenuBar 사용하지 않음)
	// 대신 ImGui::BeginChild 안에서 커스텀 버튼들로 구성

	constexpr float ToolbarHeight = 32.0f;
	constexpr float GizmoButtonSize = 24.0f;
	constexpr float GizmoIconSize = 16.0f;
	constexpr float GizmoButtonSpacing = 4.0f;

	// ViewMode labels
	const char* ViewModeLabels[] = {
		"Gouraud", "Lambert", "BlinnPhong", "Unlit", "Wireframe", "SceneDepth", "WorldNormal"
	};

	// ViewType labels
	const char* ViewTypeLabels[] = {
		"Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back"
	};

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	// ========================================
	// Left Side: Gizmo Mode Buttons
	// ========================================

	RenderGizmoModeButtons();

	// ========================================
	// World/Local Space Toggle
	// ========================================

	// QWER 버튼 다음에 8px 간격
	ImGui::SameLine(0.0f, 8.0f);

	// World/Local 토글 버튼
	if (OwningWindow)
	{
		UGizmo* ViewerGizmo = OwningWindow->GetGizmo();
		if (ViewerGizmo)
		{
			bool bIsWorldMode = ViewerGizmo->IsWorldMode();
			UTexture* CurrentSpaceIcon = bIsWorldMode ? IconWorldSpace : IconLocalSpace;
			const char* Tooltip = bIsWorldMode ? "월드 스페이스 좌표 \n좌표계를 순환하려면 클릭하거나 Ctrl+`을 누르세요"
				:"로컬 스페이스 좌표 \n좌표계를 순환하려면 클릭하거나 Ctrl+`을 누르세요";

			// 파란색 활성화 스타일 없이 렌더링
			if (DrawIconButton("##WorldLocalToggle", CurrentSpaceIcon, false, Tooltip, GizmoButtonSize, GizmoIconSize))
			{
				// World ↔ Local 토글
				if (bIsWorldMode)
				{
					ViewerGizmo->SetLocal();
				}
				else
				{
					ViewerGizmo->SetWorld();
				}
			}
		}
	}

	// ========================================
	// Rotation Snap
	// ========================================

	RenderRotationSnapControls();

	// ========================================
	// Scale Snap
	// ========================================

	RenderScaleSnapControls();

	// ========================================
	// Right Side: ViewType, Camera Settings, ViewMode
	// ========================================

	// Calculate right-aligned position
	EViewType CurrentViewType = ViewportClient->GetViewType();
	int32 CurrentViewTypeIndex = static_cast<int32>(CurrentViewType);
	constexpr float RightViewTypeButtonWidthDefault = 110.0f;
	constexpr float RightViewTypeIconSize = 16.0f;
	constexpr float RightViewTypePadding = 4.0f;

	EViewModeIndex CurrentMode = ViewportClient->GetViewMode();
	int32 CurrentModeIndex = static_cast<int32>(CurrentMode);
	constexpr float ViewModeButtonHeight = 24.0f;
	constexpr float ViewModeIconSize = 16.0f;
	constexpr float ViewModePadding = 4.0f;
	const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(ViewModeLabels[CurrentModeIndex]);
	const float ViewModeButtonWidth = ViewModePadding + ViewModeIconSize + ViewModePadding + ViewModeTextSize.x + ViewModePadding;

	constexpr float CameraSpeedButtonWidth = 70.0f;
	constexpr float GridSettingsButtonWidth = 100.0f;
	constexpr float RightButtonSpacing = 6.0f;
	const float TotalRightButtonsWidth = RightViewTypeButtonWidthDefault + RightButtonSpacing + CameraSpeedButtonWidth + RightButtonSpacing + GridSettingsButtonWidth + RightButtonSpacing + ViewModeButtonWidth;

	{
		const float ContentRegionRight = ImGui::GetWindowContentRegionMax().x;
		float RightAlignedX = ContentRegionRight - TotalRightButtonsWidth - 6.0f;
		RightAlignedX = std::max(RightAlignedX, ImGui::GetCursorPosX());

		ImGui::SameLine();
		ImGui::SetCursorPosX(RightAlignedX);
	}

	// 우측 버튼 1: ViewType
	{
		UTexture* ViewTypeIcons[7] = { IconPerspective, IconTop, IconBottom, IconLeft, IconRight, IconFront, IconBack };
		UTexture* RightViewTypeIcon = ViewTypeIcons[CurrentViewTypeIndex];

		constexpr float RightViewTypeButtonHeight = 24.0f;

		ImVec2 RightViewTypeButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##RightViewTypeButton", ImVec2(RightViewTypeButtonWidthDefault, RightViewTypeButtonHeight));
		bool bRightViewTypeClicked = ImGui::IsItemClicked();
		bool bRightViewTypeHovered = ImGui::IsItemHovered();

		ImDrawList* RightViewTypeDrawList = ImGui::GetWindowDrawList();
		ImU32 RightViewTypeBgColor = bRightViewTypeHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			RightViewTypeBgColor = IM_COL32(38, 38, 38, 255);
		}
		RightViewTypeDrawList->AddRectFilled(RightViewTypeButtonPos, ImVec2(RightViewTypeButtonPos.x + RightViewTypeButtonWidthDefault, RightViewTypeButtonPos.y + RightViewTypeButtonHeight), RightViewTypeBgColor, 4.0f);
		RightViewTypeDrawList->AddRect(RightViewTypeButtonPos, ImVec2(RightViewTypeButtonPos.x + RightViewTypeButtonWidthDefault, RightViewTypeButtonPos.y + RightViewTypeButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 아이콘
		if (RightViewTypeIcon && RightViewTypeIcon->GetTextureSRV())
		{
			const ImVec2 RightViewTypeIconPos = ImVec2(RightViewTypeButtonPos.x + RightViewTypePadding, RightViewTypeButtonPos.y + (RightViewTypeButtonHeight - RightViewTypeIconSize) * 0.5f);
			RightViewTypeDrawList->AddImage(
				RightViewTypeIcon->GetTextureSRV(),
				RightViewTypeIconPos,
				ImVec2(RightViewTypeIconPos.x + RightViewTypeIconSize, RightViewTypeIconPos.y + RightViewTypeIconSize)
			);
		}

		// 텍스트
		const ImVec2 RightViewTypeTextPos = ImVec2(RightViewTypeButtonPos.x + RightViewTypePadding + RightViewTypeIconSize + RightViewTypePadding, RightViewTypeButtonPos.y + (RightViewTypeButtonHeight - ImGui::GetTextLineHeight()) * 0.5f);
		RightViewTypeDrawList->AddText(RightViewTypeTextPos, IM_COL32(220, 220, 220, 255), ViewTypeLabels[CurrentViewTypeIndex]);

		if (bRightViewTypeClicked)
		{
			ImGui::OpenPopup("##RightViewTypeDropdown");
		}

		// ViewType 드롭다운
		if (ImGui::BeginPopup("##RightViewTypeDropdown"))
		{
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

			for (int i = 0; i < 7; ++i)
			{
				if (ViewTypeIcons[i] && ViewTypeIcons[i]->GetTextureSRV())
				{
					ImGui::Image((ImTextureID)ViewTypeIcons[i]->GetTextureSRV(), ImVec2(16, 16));
					ImGui::SameLine();
				}

				bool bIsCurrentViewType = (i == CurrentViewTypeIndex);
				if (ImGui::MenuItem(ViewTypeLabels[i], nullptr, bIsCurrentViewType))
				{
					EViewType NewType = static_cast<EViewType>(i);
					ViewportClient->SetViewType(NewType);

					// Update camera type
					if (NewType == EViewType::Perspective)
					{
						Camera->SetCameraType(ECameraType::ECT_Perspective);
					}
					else
					{
						Camera->SetCameraType(ECameraType::ECT_Orthographic);
					}
				}
			}

			ImGui::PopStyleColor();
			ImGui::EndPopup();
		}
	}

	ImGui::SameLine(0.0f, RightButtonSpacing);

	// 우측 버튼 2: Camera Settings (아이콘 + 속도 표시)
	if (IconCamera && IconCamera->GetTextureSRV())
	{
		// 카메라 속도 텍스트
		float CameraMoveSpeed = Camera->GetMoveSpeed();
		char CameraSpeedText[16];
		(void)snprintf(CameraSpeedText, sizeof(CameraSpeedText), "%.1f", CameraMoveSpeed);

		constexpr float CameraSpeedButtonHeight = 24.0f;
		constexpr float CameraSpeedPadding = 8.0f;
		constexpr float CameraSpeedIconSize = 16.0f;

		ImVec2 CameraSpeedButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##CameraSpeedButton", ImVec2(CameraSpeedButtonWidth, CameraSpeedButtonHeight));
		bool bCameraSpeedClicked = ImGui::IsItemClicked();
		bool bCameraSpeedHovered = ImGui::IsItemHovered();

		ImDrawList* CameraSpeedDrawList = ImGui::GetWindowDrawList();
		ImU32 CameraSpeedBgColor = bCameraSpeedHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			CameraSpeedBgColor = IM_COL32(38, 38, 38, 255);
		}
		CameraSpeedDrawList->AddRectFilled(CameraSpeedButtonPos, ImVec2(CameraSpeedButtonPos.x + CameraSpeedButtonWidth, CameraSpeedButtonPos.y + CameraSpeedButtonHeight), CameraSpeedBgColor, 4.0f);
		CameraSpeedDrawList->AddRect(CameraSpeedButtonPos, ImVec2(CameraSpeedButtonPos.x + CameraSpeedButtonWidth, CameraSpeedButtonPos.y + CameraSpeedButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 아이콘 (왼쪽)
		const ImVec2 CameraSpeedIconPos = ImVec2(
			CameraSpeedButtonPos.x + CameraSpeedPadding,
			CameraSpeedButtonPos.y + (CameraSpeedButtonHeight - CameraSpeedIconSize) * 0.5f
		);
		CameraSpeedDrawList->AddImage(
			IconCamera->GetTextureSRV(),
			CameraSpeedIconPos,
			ImVec2(CameraSpeedIconPos.x + CameraSpeedIconSize, CameraSpeedIconPos.y + CameraSpeedIconSize)
		);

		// 텍스트 (오른쪽 정렬)
		const ImVec2 CameraSpeedTextSize = ImGui::CalcTextSize(CameraSpeedText);
		const ImVec2 CameraSpeedTextPos = ImVec2(
			CameraSpeedButtonPos.x + CameraSpeedButtonWidth - CameraSpeedTextSize.x - CameraSpeedPadding,
			CameraSpeedButtonPos.y + (CameraSpeedButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
		);
		CameraSpeedDrawList->AddText(CameraSpeedTextPos, IM_COL32(220, 220, 220, 255), CameraSpeedText);

		if (bCameraSpeedClicked)
		{
			ImGui::OpenPopup("##CameraSettingsPopup");
		}

		// 카메라 설정 팝업
		if (ImGui::BeginPopup("##CameraSettingsPopup"))
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

			RenderCameraSpeedButton();

			ImGui::PopStyleColor(4);
			ImGui::EndPopup();
		}

		if (bCameraSpeedHovered)
		{
			ImGui::SetTooltip("Camera Settings");
		}
	}

	ImGui::SameLine(0.0f, RightButtonSpacing);

	// 우측 버튼 3: Grid Settings
	RenderGridSettingsButton();

	ImGui::SameLine(0.0f, RightButtonSpacing);

	// 우측 버튼 4: ViewMode
	{
		ImVec2 ViewModeButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ViewModeButton", ImVec2(ViewModeButtonWidth, ViewModeButtonHeight));
		bool bViewModeClicked = ImGui::IsItemClicked();
		bool bViewModeHovered = ImGui::IsItemHovered();

		ImDrawList* ViewModeDrawList = ImGui::GetWindowDrawList();
		ImU32 ViewModeBgColor = bViewModeHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			ViewModeBgColor = IM_COL32(38, 38, 38, 255);
		}
		ViewModeDrawList->AddRectFilled(ViewModeButtonPos, ImVec2(ViewModeButtonPos.x + ViewModeButtonWidth, ViewModeButtonPos.y + ViewModeButtonHeight), ViewModeBgColor, 4.0f);
		ViewModeDrawList->AddRect(ViewModeButtonPos, ImVec2(ViewModeButtonPos.x + ViewModeButtonWidth, ViewModeButtonPos.y + ViewModeButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// LitCube 아이콘
		if (IconLitCube && IconLitCube->GetTextureSRV())
		{
			const ImVec2 ViewModeIconPos = ImVec2(ViewModeButtonPos.x + ViewModePadding, ViewModeButtonPos.y + (ViewModeButtonHeight - ViewModeIconSize) * 0.5f);
			ViewModeDrawList->AddImage(
				IconLitCube->GetTextureSRV(),
				ViewModeIconPos,
				ImVec2(ViewModeIconPos.x + ViewModeIconSize, ViewModeIconPos.y + ViewModeIconSize)
			);
		}

		// 텍스트
		const ImVec2 ViewModeTextPos = ImVec2(ViewModeButtonPos.x + ViewModePadding + ViewModeIconSize + ViewModePadding, ViewModeButtonPos.y + (ViewModeButtonHeight - ImGui::GetTextLineHeight()) * 0.5f);
		ViewModeDrawList->AddText(ViewModeTextPos, IM_COL32(220, 220, 220, 255), ViewModeLabels[CurrentModeIndex]);

		if (bViewModeClicked)
		{
			ImGui::OpenPopup("##ViewModePopup");
		}

		// ViewMode 팝업
		if (ImGui::BeginPopup("##ViewModePopup"))
		{
			for (int i = 0; i < 7; ++i)
			{
				if (IconLitCube && IconLitCube->GetTextureSRV())
				{
					ImGui::Image(IconLitCube->GetTextureSRV(), ImVec2(16, 16));
					ImGui::SameLine();
				}

				if (ImGui::MenuItem(ViewModeLabels[i], nullptr, i == CurrentModeIndex))
				{
					ViewportClient->SetViewMode(static_cast<EViewModeIndex>(i));
				}
			}
			ImGui::EndPopup();
		}

		if (bViewModeHovered)
		{
			ImGui::SetTooltip("View Mode");
		}
	}

	ImGui::PopStyleVar(3);
}

void USkeletalMeshViewerToolbarWidget::RenderViewTypeButton()
{
	// This function is declared in header but not used in the extracted code
	// Implementation can be added later if needed
}

void USkeletalMeshViewerToolbarWidget::RenderCameraSpeedButton()
{
	if (!ViewportClient)
	{
		return;
	}

	UCamera* Camera = ViewportClient->GetCamera();
	if (!Camera)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Camera Settings");
	ImGui::Separator();
	ImGui::Spacing();

	// 위치
	FVector Location = Camera->GetLocation();
	float Loc[3] = { Location.X, Location.Y, Location.Z };
	if (ImGui::DragFloat3("Location", Loc, 1.0f))
	{
		Camera->SetLocation(FVector(Loc[0], Loc[1], Loc[2]));
	}

	// 회전
	FVector Rotation = Camera->GetRotation();
	float Rot[3] = { Rotation.X, Rotation.Y, Rotation.Z };
	if (ImGui::DragFloat3("Rotation", Rot, 1.0f))
	{
		Camera->SetRotation(FVector(Rot[0], Rot[1], Rot[2]));
	}

	ImGui::Spacing();

	// Perspective 전용 설정
	ECameraType CameraType = Camera->GetCameraType();
	if (CameraType == ECameraType::ECT_Perspective)
	{
		float FOV = Camera->GetFovY();
		if (ImGui::DragFloat("FOV", &FOV, 0.1f, 10.0f, 120.0f, "%.1f"))
		{
			Camera->SetFovY(FOV);
		}
	}
	// Orthographic 전용 설정
	else
	{
		float OrthoZoom = Camera->GetOrthoZoom();
		if (ImGui::SliderFloat("Ortho Zoom", &OrthoZoom, 10.0f, 10000.0f, "%.1f"))
		{
			Camera->SetOrthoZoom(OrthoZoom);
		}
	}

	ImGui::Spacing();

	// Near/Far Z 클리핑 평면
	float NearZ = Camera->GetNearZ();
	if (ImGui::DragFloat("Near Z", &NearZ, 0.01f, 0.01f, 100.0f, "%.2f"))
	{
		Camera->SetNearZ(NearZ);
	}

	float FarZ = Camera->GetFarZ();
	if (ImGui::DragFloat("Far Z", &FarZ, 10.0f, 100.0f, 100000.0f, "%.1f"))
	{
		Camera->SetFarZ(FarZ);
	}

	ImGui::Spacing();

	// 카메라 이동 속도
	float MoveSpeed = Camera->GetMoveSpeed();
	if (ImGui::DragFloat("Move Speed", &MoveSpeed, 0.1f, 0.1f, 100.0f, "%.1f"))
	{
		Camera->SetMoveSpeed(MoveSpeed);
	}
}

void USkeletalMeshViewerToolbarWidget::RenderViewModeButton()
{
	// This function is declared in header but not used in the extracted code
	// Implementation can be added later if needed
}

void USkeletalMeshViewerToolbarWidget::RenderGridSettingsButton()
{
	if (!OwningWindow)
	{
		return;
	}

	// 버튼 크기 및 스타일 설정
	constexpr float GridButtonWidth = 100.0f;
	constexpr float GridButtonHeight = 24.0f;
	constexpr float GridPadding = 8.0f;
	constexpr float GridIconSize = 16.0f;

	const char* GridText = "Grid";

	ImVec2 GridButtonPos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##GridButton", ImVec2(GridButtonWidth, GridButtonHeight));
	bool bGridClicked = ImGui::IsItemClicked();
	bool bGridHovered = ImGui::IsItemHovered();

	ImDrawList* GridDrawList = ImGui::GetWindowDrawList();
	ImU32 GridBgColor = bGridHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
	if (ImGui::IsItemActive())
	{
		GridBgColor = IM_COL32(38, 38, 38, 255);
	}
	GridDrawList->AddRectFilled(GridButtonPos, ImVec2(GridButtonPos.x + GridButtonWidth, GridButtonPos.y + GridButtonHeight), GridBgColor, 4.0f);
	GridDrawList->AddRect(GridButtonPos, ImVec2(GridButtonPos.x + GridButtonWidth, GridButtonPos.y + GridButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

	// 그리드 아이콘 (간단한 그리드 패턴)
	const ImVec2 GridIconPos = ImVec2(
		GridButtonPos.x + GridPadding,
		GridButtonPos.y + (GridButtonHeight - GridIconSize) * 0.5f
	);

	// 작은 그리드 패턴 그리기
	const float CellSize = GridIconSize / 3.0f;
	for (int x = 0; x < 3; ++x)
	{
		for (int y = 0; y < 3; ++y)
		{
			ImVec2 CellMin = ImVec2(GridIconPos.x + x * CellSize, GridIconPos.y + y * CellSize);
			ImVec2 CellMax = ImVec2(CellMin.x + CellSize, CellMin.y + CellSize);
			GridDrawList->AddRect(CellMin, CellMax, IM_COL32(180, 180, 180, 255), 0.0f, 0, 1.0f);
		}
	}

	// 텍스트 (오른쪽 정렬)
	const ImVec2 GridTextSize = ImGui::CalcTextSize(GridText);
	const ImVec2 GridTextPos = ImVec2(
		GridButtonPos.x + GridButtonWidth - GridTextSize.x - GridPadding,
		GridButtonPos.y + (GridButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
	);
	GridDrawList->AddText(GridTextPos, IM_COL32(220, 220, 220, 255), GridText);

	if (bGridClicked)
	{
		ImGui::OpenPopup("##GridSettingsPopup");
	}

	// 그리드 설정 팝업
	if (ImGui::BeginPopup("##GridSettingsPopup"))
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Grid Settings");
		ImGui::Separator();
		ImGui::Spacing();

		float GridCellSize = OwningWindow->GetGridCellSize();
		if (ImGui::DragFloat("Cell Size", &GridCellSize, 0.01f, 0.1f, 10.f, "%.1f"))
		{
			OwningWindow->SetGridCellSize(GridCellSize);
		}

		ImGui::PopStyleColor(4);
		ImGui::EndPopup();
	}

	if (bGridHovered)
	{
		ImGui::SetTooltip("Grid Settings");
	}
}
