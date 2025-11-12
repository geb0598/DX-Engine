#include "pch.h"
#include "Render/UI/Widget/Public/ViewportControlWidget.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/EditorEngine.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Texture/Public/Texture.h"
#include "Actor/Public/Actor.h"

// 정적 멤버 정의 - KTLWeek07의 뷰 모드 기능 사용
const char* UViewportControlWidget::ViewModeLabels[] = {
	"Gouraud", "Lambert", "BlinnPhong", "Unlit", "Wireframe", "SceneDepth", "WorldNormal"
};

const char* UViewportControlWidget::ViewTypeLabels[] = {
	"Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back"
};

UViewportControlWidget::UViewportControlWidget()
	: UViewportToolbarWidgetBase()
{
	SetName("Viewport Control Widget");
}

UViewportControlWidget::~UViewportControlWidget() = default;

void UViewportControlWidget::Initialize()
{
	LoadCommonIcons();
	LoadViewportSpecificIcons();
	UE_LOG("ViewportControlWidget: Initialized");
}

void UViewportControlWidget::LoadViewportSpecificIcons()
{
	UE_LOG("ViewportControlWidget: 전용 아이콘 로드 시작...");
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UPathManager& PathManager = UPathManager::GetInstance();
	FString IconBasePath = PathManager.GetAssetPath().string() + "\\Icon\\";

	int32 LoadedCount = 0;

	// 레이아웃 전환 아이콘 로드 (ViewportControl 전용)
	IconQuad = AssetManager.LoadTexture((IconBasePath + "quad.png").data());
	if (IconQuad) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'quad' -> %p", IconQuad);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "quad.png").c_str());
	}

	IconSquare = AssetManager.LoadTexture((IconBasePath + "square.png").data());
	if (IconSquare) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'square' -> %p", IconSquare);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "square.png").c_str());
	}

	UE_LOG_SUCCESS("ViewportControlWidget: 전용 아이콘 로드 완료 (%d/2)", LoadedCount);
}

void UViewportControlWidget::Update()
{
	// 필요시 업데이트 로직 추가

}

void UViewportControlWidget::RenderWidget()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	if (!ViewportManager.GetRoot())
	{
		return;
	}

	// 먼저 스플리터 선 렌더링 (UI 뒤에서)
	//RenderSplitterLines();

	// 싱글 모드에서는 하나만 렌더링
	if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
	{
		int32 ActiveViewportIndex = ViewportManager.GetActiveIndex();
		RenderViewportToolbar(ActiveViewportIndex);
	}
	else
	{
		for (int32 i = 0;i < 4;++i)
		{
			RenderViewportToolbar(i);
		}
	}
}

void UViewportControlWidget::RenderViewportToolbar(int32 ViewportIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	// 뷰포트 범위가 벗어날 경우 종료
	if (ViewportIndex >= Viewports.Num() || ViewportIndex >= Clients.Num())
	{
		return;
	}

	// FutureEngine: null 체크 추가
	if (!Viewports[ViewportIndex] || !Clients[ViewportIndex])
	{
		return;
	}

	const FRect& Rect = Viewports[ViewportIndex]->GetRect();
	if (Rect.Width <= 0 || Rect.Height <= 0)
	{
		return;
	}


	constexpr int32 ToolbarH = 32;
	const ImVec2 Vec1{ static_cast<float>(Rect.Left), static_cast<float>(Rect.Top) };
	const ImVec2 Vec2{ static_cast<float>(Rect.Left + Rect.Width), static_cast<float>(Rect.Top + ToolbarH) };

	// 1) 툴바 배경 그리기
	ImDrawList* DrawLine = ImGui::GetBackgroundDrawList();
	DrawLine->AddRectFilled(Vec1, Vec2, IM_COL32(30, 30, 30, 100));
	DrawLine->AddLine(ImVec2(Vec1.x, Vec2.y), ImVec2(Vec2.x, Vec2.y), IM_COL32(70, 70, 70, 120), 1.0f);

	// 2) 툴바 UI 요소들을 직접 배치 (별도 창 생성하지 않음)
	// UI 요소들을 화면 좌표계에서 직접 배치
	ImGui::SetNextWindowPos(ImVec2(Vec1.x, Vec1.y));
	ImGui::SetNextWindowSize(ImVec2(Vec2.x - Vec1.x, Vec2.y - Vec1.y));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoTitleBar;

	// 스타일 설정
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	char WinName[64];
	(void)snprintf(WinName, sizeof(WinName), "##ViewportToolbar%d", ViewportIndex);

	ImGui::PushID(ViewportIndex);
	if (ImGui::Begin(WinName, nullptr, flags))
	{
		// ========================================
		// PIE 상태 확인
		// ========================================
		const bool bIsPIEActive = (GEditor && GEditor->IsPIESessionActive());
		const bool bIsPIEMouseDetached = (GEditor && GEditor->IsPIEMouseDetached());
		const bool bIsPIEViewport = (bIsPIEActive && ViewportIndex == ViewportManager.GetPIEActiveViewportIndex());

		// ========================================
		// Gizmo Mode 버튼들 (Select/Translate/Rotate/Scale)
		// ========================================
		UEditor* Editor = GEditor ? GEditor->GetEditorModule() : nullptr;
		UGizmo* Gizmo = Editor ? Editor->GetGizmo() : nullptr;
		EGizmoMode CurrentGizmoMode = Gizmo ? Gizmo->GetGizmoMode() : EGizmoMode::Translate;

		constexpr float GizmoButtonSize = 24.0f;
		constexpr float GizmoIconSize = 16.0f;
		constexpr float GizmoButtonSpacing = 4.0f;

		// 기즈모 버튼 렌더링
		if (!bIsPIEViewport)
		{
			// Select 버튼 (아직 기능 없음 - 미연결)
			if (IconSelect && IconSelect->GetTextureSRV())
			{
				bool bActive = false; // Select 모드는 아직 없음
				ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##GizmoSelect", ImVec2(GizmoButtonSize, GizmoButtonSize));
				bool bClicked = ImGui::IsItemClicked();
				bool bHovered = ImGui::IsItemHovered();

				ImDrawList* DL = ImGui::GetWindowDrawList();
				ImU32 BgColor = bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

				DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BgColor, 4.0f);
				DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 (중앙 정렬)
				ImVec2 IconPos = ImVec2(ButtonPos.x + (GizmoButtonSize - GizmoIconSize) * 0.5f, ButtonPos.y + (GizmoButtonSize - GizmoIconSize) * 0.5f);
				DL->AddImage(IconSelect->GetTextureSRV(), IconPos, ImVec2(IconPos.x + GizmoIconSize, IconPos.y + GizmoIconSize));

				if (bHovered) ImGui::SetTooltip("Select (Q)");
			}

			ImGui::SameLine(0.0f, GizmoButtonSpacing);

			// Translate 버튼
			if (IconTranslate && IconTranslate->GetTextureSRV())
			{
				bool bActive = (CurrentGizmoMode == EGizmoMode::Translate);
				ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##GizmoTranslate", ImVec2(GizmoButtonSize, GizmoButtonSize));
				bool bClicked = ImGui::IsItemClicked();
				bool bHovered = ImGui::IsItemHovered();

				ImDrawList* DL = ImGui::GetWindowDrawList();
				ImU32 BgColor = bActive ? IM_COL32(20, 20, 20, 255) : (bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255));
				if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

				DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BgColor, 4.0f);
				ImU32 BorderColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(96, 96, 96, 255);
				DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BorderColor, 4.0f);

				// 아이콘 (활성화 시 파란색 틴트)
				ImVec2 IconPos = ImVec2(ButtonPos.x + (GizmoButtonSize - GizmoIconSize) * 0.5f, ButtonPos.y + (GizmoButtonSize - GizmoIconSize) * 0.5f);
				ImU32 TintColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(255, 255, 255, 255);
				DL->AddImage(IconTranslate->GetTextureSRV(), IconPos, ImVec2(IconPos.x + GizmoIconSize, IconPos.y + GizmoIconSize), ImVec2(0, 0), ImVec2(1, 1), TintColor);

				if (bClicked && Gizmo) Gizmo->SetGizmoMode(EGizmoMode::Translate);
				if (bHovered) ImGui::SetTooltip("Translate (W)");
			}

			ImGui::SameLine(0.0f, GizmoButtonSpacing);

			// Rotate 버튼
			if (IconRotate && IconRotate->GetTextureSRV())
			{
				bool bActive = (CurrentGizmoMode == EGizmoMode::Rotate);
				ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##GizmoRotate", ImVec2(GizmoButtonSize, GizmoButtonSize));
				bool bClicked = ImGui::IsItemClicked();
				bool bHovered = ImGui::IsItemHovered();

				ImDrawList* DL = ImGui::GetWindowDrawList();
				ImU32 BgColor = bActive ? IM_COL32(20, 20, 20, 255) : (bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255));
				if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

				DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BgColor, 4.0f);
				ImU32 BorderColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(96, 96, 96, 255);
				DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BorderColor, 4.0f);

				ImVec2 IconPos = ImVec2(ButtonPos.x + (GizmoButtonSize - GizmoIconSize) * 0.5f, ButtonPos.y + (GizmoButtonSize - GizmoIconSize) * 0.5f);
				ImU32 TintColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(255, 255, 255, 255);
				DL->AddImage(IconRotate->GetTextureSRV(), IconPos, ImVec2(IconPos.x + GizmoIconSize, IconPos.y + GizmoIconSize), ImVec2(0, 0), ImVec2(1, 1), TintColor);

				if (bClicked && Gizmo) Gizmo->SetGizmoMode(EGizmoMode::Rotate);
				if (bHovered) ImGui::SetTooltip("Rotate (E)");
			}

			ImGui::SameLine(0.0f, GizmoButtonSpacing);

			// Scale 버튼
			if (IconScale && IconScale->GetTextureSRV())
			{
				bool bActive = (CurrentGizmoMode == EGizmoMode::Scale);
				ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##GizmoScale", ImVec2(GizmoButtonSize, GizmoButtonSize));
				bool bClicked = ImGui::IsItemClicked();
				bool bHovered = ImGui::IsItemHovered();

				ImDrawList* DL = ImGui::GetWindowDrawList();
				ImU32 BgColor = bActive ? IM_COL32(20, 20, 20, 255) : (bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255));
				if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

				DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BgColor, 4.0f);
				ImU32 BorderColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(96, 96, 96, 255);
				DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BorderColor, 4.0f);

				ImVec2 IconPos = ImVec2(ButtonPos.x + (GizmoButtonSize - GizmoIconSize) * 0.5f, ButtonPos.y + (GizmoButtonSize - GizmoIconSize) * 0.5f);
				ImU32 TintColor = bActive ? IM_COL32(46, 163, 255, 255) : IM_COL32(255, 255, 255, 255);
				DL->AddImage(IconScale->GetTextureSRV(), IconPos, ImVec2(IconPos.x + GizmoIconSize, IconPos.y + GizmoIconSize), ImVec2(0, 0), ImVec2(1, 1), TintColor);

				if (bClicked && Gizmo) Gizmo->SetGizmoMode(EGizmoMode::Scale);
				if (bHovered) ImGui::SetTooltip("Scale (R)");
			}

			// ========================================
			// World/Local Space Toggle
			// ========================================

			ImGui::SameLine(0.0f, 8.0f);

			// World/Local 토글 버튼
			if (Gizmo && IconWorldSpace && IconWorldSpace->GetTextureSRV() && IconLocalSpace && IconLocalSpace->GetTextureSRV())
			{
				bool bIsWorldMode = Gizmo->IsWorldMode();
				UTexture* CurrentSpaceIcon = bIsWorldMode ? IconWorldSpace : IconLocalSpace;
				const char* Tooltip = bIsWorldMode ? "월드 스페이스 좌표 \n좌표계를 순환하려면 클릭하거나 Ctrl+`을 누르세요"
					:"로컬 스페이스 좌표 \n 좌표계를 순환하려면 클릭하거나 Ctrl+`을 누르세요";

				ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##WorldLocalToggle", ImVec2(GizmoButtonSize, GizmoButtonSize));
				bool bClicked = ImGui::IsItemClicked();
				bool bHovered = ImGui::IsItemHovered();

				ImDrawList* DL = ImGui::GetWindowDrawList();
				// 다른 버튼들과 동일한 스타일: 기본 검은색, 호버 시 어두운 회색, 클릭 시 더 밝은 회색
				ImU32 BgColor = bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive()) BgColor = IM_COL32(38, 38, 38, 255);

				DL->AddRectFilled(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BgColor, 4.0f);
				ImU32 BorderColor = IM_COL32(96, 96, 96, 255);
				DL->AddRect(ButtonPos, ImVec2(ButtonPos.x + GizmoButtonSize, ButtonPos.y + GizmoButtonSize), BorderColor, 4.0f);

				ImVec2 IconPos = ImVec2(ButtonPos.x + (GizmoButtonSize - GizmoIconSize) * 0.5f, ButtonPos.y + (GizmoButtonSize - GizmoIconSize) * 0.5f);
				ImU32 TintColor = IM_COL32(255, 255, 255, 255);
				DL->AddImage(CurrentSpaceIcon->GetTextureSRV(), IconPos, ImVec2(IconPos.x + GizmoIconSize, IconPos.y + GizmoIconSize), ImVec2(0, 0), ImVec2(1, 1), TintColor);

				if (bClicked)
				{
					// World ↔ Local 토글
					if (bIsWorldMode)
					{
						Gizmo->SetLocal();
					}
					else
					{
						Gizmo->SetWorld();
					}
				}

				if (bHovered) ImGui::SetTooltip("%s", Tooltip);
			}

			// ========================================
			// Location Snap 버튼들
			// ========================================
			bool bLocationSnapEnabled = ViewportManager.IsLocationSnapEnabled();
			float LocationSnapValue = ViewportManager.GetLocationSnapValue();
			float OldLocationSnapValue = LocationSnapValue;

			// Base 클래스의 공통 렌더링 함수 사용
			RenderLocationSnapControls(bLocationSnapEnabled, LocationSnapValue);

			// 값이 변경되었으면 ViewportManager에 반영 및 Grid 동기화
			if (bLocationSnapEnabled != ViewportManager.IsLocationSnapEnabled())
			{
				ViewportManager.SetLocationSnapEnabled(bLocationSnapEnabled);
			}
			if (std::abs(LocationSnapValue - OldLocationSnapValue) > 0.01f)
			{
				ViewportManager.SetLocationSnapValue(LocationSnapValue);

				// Grid 크기 동기화
				if (Editor)
				{
					UBatchLines* BatchLine = Editor->GetBatchLines();
					if (BatchLine)
					{
						BatchLine->UpdateUGridVertices(LocationSnapValue);
					}
				}
			}
		}
		else
		{
			// PIE 상태 텍스트 표시
			if (!bIsPIEMouseDetached)
			{
				// Dummy 공간 확보
				ImGui::Dummy(ImVec2(0.0f, GizmoButtonSize));

				// DrawList로 직접 텍스트 렌더링
				ImDrawList* DL = ImGui::GetWindowDrawList();
				const float TextHeight = ImGui::GetTextLineHeight();
				const float ButtonCenterOffset = (GizmoButtonSize - TextHeight) * 0.5f;
				ImVec2 TextPos = ImGui::GetCursorScreenPos();
				TextPos.y = TextPos.y - GizmoButtonSize + ButtonCenterOffset;

				DL->AddText(TextPos, IM_COL32(200, 200, 100, 255), "Playing (Shift + F1 to detach mouse)");
			}
		}

		// ========================================
		// Rotation Snap 버튼들
		// ========================================
		if (!bIsPIEViewport)
		{
			bool bRotationSnapEnabled = ViewportManager.IsRotationSnapEnabled();
			float RotationSnapAngle = ViewportManager.GetRotationSnapAngle();

			// Base 클래스의 공통 렌더링 함수 사용
			RenderRotationSnapControls(bRotationSnapEnabled, RotationSnapAngle);

			// 값이 변경되었으면 ViewportManager에 반영
			if (bRotationSnapEnabled != ViewportManager.IsRotationSnapEnabled())
			{
				ViewportManager.SetRotationSnapEnabled(bRotationSnapEnabled);
			}
			if (std::abs(RotationSnapAngle - ViewportManager.GetRotationSnapAngle()) > 0.01f)
			{
				ViewportManager.SetRotationSnapAngle(RotationSnapAngle);
			}
		}

		// ========================================
		// Scale Snap 버튼들
		// ========================================
		if (!bIsPIEViewport)
		{
			bool bScaleSnapEnabled = ViewportManager.IsScaleSnapEnabled();
			float ScaleSnapValue = ViewportManager.GetScaleSnapValue();

			// Base 클래스의 공통 렌더링 함수 사용
			RenderScaleSnapControls(bScaleSnapEnabled, ScaleSnapValue);

			// 값이 변경되었으면 ViewportManager에 반영
			if (bScaleSnapEnabled != ViewportManager.IsScaleSnapEnabled())
			{
				ViewportManager.SetScaleSnapEnabled(bScaleSnapEnabled);
			}
			if (std::abs(ScaleSnapValue - ViewportManager.GetScaleSnapValue()) > 0.0001f)
			{
				ViewportManager.SetScaleSnapValue(ScaleSnapValue);
			}
		}

		// 우측 정렬할 버튼들의 총 너비 계산
		bool bInPilotMode = IsPilotModeActive(ViewportIndex);
		AActor* PilotedActor = (Editor && bInPilotMode) ? Editor->GetPilotedActor() : nullptr;

		// ViewType 버튼 폭 계산
		constexpr float RightViewTypeButtonWidthDefault = 110.0f;
		constexpr float RightViewTypeIconSize = 16.0f;
		constexpr float RightViewTypePadding = 4.0f;
		float RightViewTypeButtonWidth = RightViewTypeButtonWidthDefault;

		// Actor 이름 저장용
		static FString CachedActorName;

		if (bInPilotMode && PilotedActor)
		{
			CachedActorName = PilotedActor->GetName().ToString();
			const ImVec2 ActorNameTextSize = ImGui::CalcTextSize(CachedActorName.c_str());
			// 아이콘 + 패딩 + 텍스트 + 패딩
			RightViewTypeButtonWidth = RightViewTypePadding + RightViewTypeIconSize + RightViewTypePadding + ActorNameTextSize.x + RightViewTypePadding;
			// 최소 / 최대 폭 제한
			RightViewTypeButtonWidth = Clamp(RightViewTypeButtonWidth, 110.0f, 250.0f);
		}

		// ViewMode 버튼 폭 계산
		EViewModeIndex CurrentMode = Clients[ViewportIndex]->GetViewMode();
		int32 CurrentModeIndex = static_cast<int32>(CurrentMode);
		constexpr float ViewModeButtonHeight = 24.0f;
		constexpr float ViewModeIconSize = 16.0f;
		constexpr float ViewModePadding = 4.0f;
		const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(ViewModeLabels[CurrentModeIndex]);
		const float ViewModeButtonWidth = ViewModePadding + ViewModeIconSize + ViewModePadding + ViewModeTextSize.x + ViewModePadding;

		constexpr float CameraSpeedButtonWidth = 70.0f; // 아이콘 + 숫자 표시를 위해 확장
		constexpr float LayoutToggleButtonSize = 24.0f;
		constexpr float RightButtonSpacing = 6.0f;
		constexpr float PilotExitButtonSize = 24.0f;
		const float PilotModeExtraWidth = bInPilotMode ? (PilotExitButtonSize + RightButtonSpacing) : 0.0f;
		const float TotalRightButtonsWidth = RightViewTypeButtonWidth + RightButtonSpacing + PilotModeExtraWidth + CameraSpeedButtonWidth + RightButtonSpacing + ViewModeButtonWidth + RightButtonSpacing + LayoutToggleButtonSize;

		// 우측 정렬 시작
		{
			const float ContentRegionRight = ImGui::GetWindowContentRegionMax().x;
			float RightAlignedX = ContentRegionRight - TotalRightButtonsWidth - 6.0f;
			RightAlignedX = std::max(RightAlignedX, ImGui::GetCursorPosX());

			ImGui::SameLine();
			ImGui::SetCursorPosX(RightAlignedX);
		}

		// 우측 버튼 1: 카메라 뷰 타입 (파일럿 모드 지원)
		{
			// 현재 뷰 타입 정보
			EViewType CurrentViewType = Clients[ViewportIndex]->GetViewType();
			int32 CurrentViewTypeIndex = static_cast<int32>(CurrentViewType);
			UTexture* ViewTypeIcons[7] = { IconPerspective, IconTop, IconBottom, IconLeft, IconRight, IconFront, IconBack };

			// 현재 뷰 타입에 맞는 아이콘 가져오기
			UTexture* RightViewTypeIcon = nullptr;
			switch (CurrentViewType)
			{
				case EViewType::Perspective: RightViewTypeIcon = IconPerspective; break;
				case EViewType::OrthoTop: RightViewTypeIcon = IconTop; break;
				case EViewType::OrthoBottom: RightViewTypeIcon = IconBottom; break;
				case EViewType::OrthoLeft: RightViewTypeIcon = IconLeft; break;
				case EViewType::OrthoRight: RightViewTypeIcon = IconRight; break;
				case EViewType::OrthoFront: RightViewTypeIcon = IconFront; break;
				case EViewType::OrthoBack: RightViewTypeIcon = IconBack; break;
			}

			// 표시할 텍스트 (파일럿 모드면 Actor 이름, 아니면 ViewType)
			static char TruncatedActorName[128] = "";
			const char* DisplayText = ViewTypeLabels[CurrentViewTypeIndex];

			if (bInPilotMode && PilotedActor && !CachedActorName.empty())
			{
				// 최대 표시 가능한 텍스트 폭 계산 (버튼 폭 - 아이콘 - 패딩들)
				const float MaxTextWidth = 250.0f - RightViewTypePadding - RightViewTypeIconSize - RightViewTypePadding * 2;
				const ImVec2 ActorNameSize = ImGui::CalcTextSize(CachedActorName.c_str());

				if (ActorNameSize.x > MaxTextWidth)
				{
					// 텍스트가 너무 길면 "..." 축약
					size_t Len = CachedActorName.length();

					// "..." 제외한 텍스트 길이를 줄여가며 맞춤
					while (Len > 3)
					{
						std::string Truncated = CachedActorName.substr(0, Len - 3) + "...";
						ImVec2 TruncatedSize = ImGui::CalcTextSize(Truncated.c_str());
						if (TruncatedSize.x <= MaxTextWidth)
						{
							(void)snprintf(TruncatedActorName, sizeof(TruncatedActorName), "%s", Truncated.c_str());
							break;
						}
						Len--;
					}
					DisplayText = TruncatedActorName;
				}
				else
				{
					DisplayText = CachedActorName.c_str();
				}
			}

			constexpr float RightViewTypeButtonHeight = 24.0f;

			ImVec2 RightViewTypeButtonPos = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("##RightViewTypeButton", ImVec2(RightViewTypeButtonWidth, RightViewTypeButtonHeight));
			bool bRightViewTypeClicked = ImGui::IsItemClicked();
			bool bRightViewTypeHovered = ImGui::IsItemHovered();

			// 버튼 배경 그리기 (파일럿 모드면 강조 색상)
			ImDrawList* RightViewTypeDrawList = ImGui::GetWindowDrawList();
			ImU32 RightViewTypeBgColor;
			if (bInPilotMode)
			{
				RightViewTypeBgColor = bRightViewTypeHovered ? IM_COL32(40, 60, 80, 255) : IM_COL32(30, 50, 70, 255);
			}
			else
			{
				RightViewTypeBgColor = bRightViewTypeHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
			}
			if (ImGui::IsItemActive())
			{
				RightViewTypeBgColor = bInPilotMode ? IM_COL32(50, 70, 90, 255) : IM_COL32(38, 38, 38, 255);
			}
			RightViewTypeDrawList->AddRectFilled(RightViewTypeButtonPos, ImVec2(RightViewTypeButtonPos.x + RightViewTypeButtonWidth, RightViewTypeButtonPos.y + RightViewTypeButtonHeight), RightViewTypeBgColor, 4.0f);
			RightViewTypeDrawList->AddRect(RightViewTypeButtonPos, ImVec2(RightViewTypeButtonPos.x + RightViewTypeButtonWidth, RightViewTypeButtonPos.y + RightViewTypeButtonHeight), bInPilotMode ? IM_COL32(100, 150, 200, 255) : IM_COL32(96, 96, 96, 255), 4.0f);

			// 아이콘 그리기
			if (RightViewTypeIcon && RightViewTypeIcon->GetTextureSRV())
			{
				const ImVec2 RightViewTypeIconPos = ImVec2(RightViewTypeButtonPos.x + RightViewTypePadding, RightViewTypeButtonPos.y + (RightViewTypeButtonHeight - RightViewTypeIconSize) * 0.5f);
				RightViewTypeDrawList->AddImage(
					RightViewTypeIcon->GetTextureSRV(),
					RightViewTypeIconPos,
					ImVec2(RightViewTypeIconPos.x + RightViewTypeIconSize, RightViewTypeIconPos.y + RightViewTypeIconSize)
				);
			}

			// 텍스트 그리기
			const ImVec2 RightViewTypeTextPos = ImVec2(RightViewTypeButtonPos.x + RightViewTypePadding + RightViewTypeIconSize + RightViewTypePadding, RightViewTypeButtonPos.y + (RightViewTypeButtonHeight - ImGui::GetTextLineHeight()) * 0.5f);
			RightViewTypeDrawList->AddText(RightViewTypeTextPos, bInPilotMode ? IM_COL32(180, 220, 255, 255) : IM_COL32(220, 220, 220, 255), DisplayText);

			if (bRightViewTypeClicked)
			{
				ImGui::OpenPopup("##RightViewTypeDropdown");
			}

			// ViewType 드롭다운 팝업
			if (ImGui::BeginPopup("##RightViewTypeDropdown"))
			{
				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

				// 파일럿 모드 항목 (최상단, 선택된 상태로 표시)
				if (bInPilotMode && PilotedActor && !CachedActorName.empty())
				{
					if (IconPerspective && IconPerspective->GetTextureSRV())
					{
						ImGui::Image((ImTextureID)IconPerspective->GetTextureSRV(), ImVec2(16, 16));
						ImGui::SameLine();
					}

					// 선택된 상태로 표시 (체크마크) - CachedActorName 사용
					ImGui::MenuItem(CachedActorName.c_str(), nullptr, true, false); // 선택됨, 비활성화

					ImGui::Separator();
				}

				// 일반 ViewType 항목들
				for (int i = 0; i < IM_ARRAYSIZE(ViewTypeLabels); ++i)
				{
					if (ViewTypeIcons[i] && ViewTypeIcons[i]->GetTextureSRV())
					{
						ImGui::Image((ImTextureID)ViewTypeIcons[i]->GetTextureSRV(), ImVec2(16, 16));
						ImGui::SameLine();
					}

					bool bIsCurrentViewType = (i == CurrentViewTypeIndex && !bInPilotMode);
					if (ImGui::MenuItem(ViewTypeLabels[i], nullptr, bIsCurrentViewType))
					{
						// ViewType 변경 시 파일럿 모드 종료
						if (bInPilotMode && Editor)
						{
							Editor->RequestExitPilotMode();
						}

						EViewType NewType = static_cast<EViewType>(i);
						Clients[ViewportIndex]->SetViewType(NewType);
						UE_LOG("ViewportControlWidget: Viewport[%d]의 ViewType을 %s로 변경",
							ViewportIndex, ViewTypeLabels[i]);
					}
				}

				ImGui::PopStyleColor();
				ImGui::EndPopup();
			}
		}

		ImGui::SameLine(0.0f, RightButtonSpacing);

		// 파일럿 모드 Exit 버튼 (ViewType 버튼 옆)
		if (IsPilotModeActive(ViewportIndex))
		{
			ImVec2 ExitButtonCursorPos = ImGui::GetCursorScreenPos();
			RenderPilotModeExitButton(ViewportIndex, ExitButtonCursorPos);
			ImGui::SetCursorScreenPos(ExitButtonCursorPos);
			ImGui::SameLine(0.0f, RightButtonSpacing);
		}

		// 우측 버튼 2: 카메라 속도 (Base 클래스 함수 사용)
		if (UCamera* Camera = Clients[ViewportIndex]->GetCamera())
		{
			RenderCameraSpeedButton(Camera, CameraSpeedButtonWidth);

			// 카메라 설정 팝업
			if (ImGui::BeginPopup("##CameraSettingsPopup"))
			{
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

				if (UCamera* Camera = Clients[ViewportIndex]->GetCamera())
				{
					EViewType PopupViewType = Clients[ViewportIndex]->GetViewType();
					const bool bIsPerspective = (PopupViewType == EViewType::Perspective);
					ImGui::Text(bIsPerspective ? "Perspective Camera Settings" : "Orthographic Camera Settings");
					ImGui::Separator();

					// 전역 카메라 이동 속도
					float editorCameraSpeed = ViewportManager.GetEditorCameraSpeed();
					if (ImGui::DragFloat("Camera Speed", &editorCameraSpeed, 0.1f,
						UViewportManager::MIN_CAMERA_SPEED, UViewportManager::MAX_CAMERA_SPEED, "%.1f"))
					{
						ViewportManager.SetEditorCameraSpeed(editorCameraSpeed);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("Applies to all viewports");
					}

					// 카메라 위치
					FVector location = Camera->GetLocation();
					if (ImGui::DragFloat3("Location", &location.X, 0.1f))
					{
						Camera->SetLocation(location);
					}

					// 카메라 회전 (Perspective만 표시)
					if (bIsPerspective)
					{
						static FVector CachedRotation = FVector::ZeroVector();
						static bool bIsDraggingRotation = false;

						// 드래그 시작 시 또는 비활성 상태일 때 현재 값 캐싱
						if (!bIsDraggingRotation)
						{
							CachedRotation = Camera->GetRotation();
						}

						bool bRotationChanged = ImGui::DragFloat3("Rotation", &CachedRotation.X, 0.5f);

						// 드래그 상태 추적
						if (ImGui::IsItemActive())
						{
							bIsDraggingRotation = true;

							// 값이 변경되었으면 카메라에 반영
							if (bRotationChanged)
							{
								Camera->SetRotation(CachedRotation);
								// SetRotation 후 wrapping된 값으로 즉시 재동기화
								CachedRotation = Camera->GetRotation();
							}
						}
						else if (bIsDraggingRotation)
						{
							// 드래그 종료 시 최종 동기화
							bIsDraggingRotation = false;
							CachedRotation = Camera->GetRotation();
						}
					}
					// Orthographic 뷰는 회전 항목 없음 (고정된 방향)

					ImGui::Separator();

					if (bIsPerspective)
					{
						// Perspective: FOV 표시
						float Fov = Camera->GetFovY();
						if (ImGui::DragFloat("FOV", &Fov, 0.1f, 1.0f, 170.0f, "%.1f"))
						{
							Camera->SetFovY(Fov);
						}
					}
					else
					{
						// Orthographic: Zoom Level (OrthoZoom) 표시 및 SharedOrthoZoom 동기화
						float OrthoZoom = Camera->GetOrthoZoom();
						if (ImGui::DragFloat("Zoom Level", &OrthoZoom, 10.0f, 10.0f, 10000.0f, "%.1f"))
						{
							Camera->SetOrthoZoom(OrthoZoom);

							// 모든 Ortho 카메라에 동일한 줌 적용 (SharedOrthoZoom 갱신)
							for (FViewportClient* OtherClient : ViewportManager.GetClients())
							{
								if (OtherClient && OtherClient->IsOrtho())
								{
									if (UCamera* OtherCam = OtherClient->GetCamera())
									{
										OtherCam->SetOrthoZoom(OrthoZoom);
									}
								}
							}
						}

						// 정보: Aspect는 자동 계산됨
						float Aspect = Camera->GetAspect();
						ImGui::BeginDisabled();
						ImGui::DragFloat("Aspect Ratio", &Aspect, 0.01f, 0.1f, 10.0f, "%.3f");
						ImGui::EndDisabled();
					}

					// Near/Far Plane
					float NearZ = Camera->GetNearZ();
					if (ImGui::DragFloat("Near Z", &NearZ, 0.01f, 0.01f, 100.0f, "%.3f"))
					{
						Camera->SetNearZ(NearZ);
					}

					float FarZ = Camera->GetFarZ();
					if (ImGui::DragFloat("Far Z", &FarZ, 1.0f, 1.0f, 10000.0f, "%.1f"))
					{
						Camera->SetFarZ(FarZ);
					}
				}

				ImGui::PopStyleColor(4);
				ImGui::EndPopup();
			}
		}

		ImGui::SameLine(0.0f, RightButtonSpacing);

		// 우측 버튼 3: ViewMode 버튼 (Base 클래스 함수 사용)
		RenderViewModeButton(Clients[ViewportIndex], ViewModeLabels);

		ImGui::SameLine(0.0f, RightButtonSpacing);

		// 우측 버튼 4: 레이아웃 토글 버튼
		constexpr float LayoutToggleIconSize = 16.0f;

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
		{
			// Quad 레이아웃으로 전환 버튼
			if (IconQuad && IconQuad->GetTextureSRV())
			{
				const ImVec2 LayoutToggleQuadButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##LayoutToggleQuadButton", ImVec2(LayoutToggleButtonSize, LayoutToggleButtonSize));
				const bool bLayoutToggleQuadClicked = ImGui::IsItemClicked();
				const bool bLayoutToggleQuadHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* LayoutToggleQuadDrawList = ImGui::GetWindowDrawList();
				ImU32 LayoutToggleQuadBgColor = bLayoutToggleQuadHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					LayoutToggleQuadBgColor = IM_COL32(38, 38, 38, 255);
				}
				LayoutToggleQuadDrawList->AddRectFilled(LayoutToggleQuadButtonPos, ImVec2(LayoutToggleQuadButtonPos.x + LayoutToggleButtonSize, LayoutToggleQuadButtonPos.y + LayoutToggleButtonSize), LayoutToggleQuadBgColor, 4.0f);
				LayoutToggleQuadDrawList->AddRect(LayoutToggleQuadButtonPos, ImVec2(LayoutToggleQuadButtonPos.x + LayoutToggleButtonSize, LayoutToggleQuadButtonPos.y + LayoutToggleButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (중앙 정렬)
				const ImVec2 LayoutToggleQuadIconPos = ImVec2(
					LayoutToggleQuadButtonPos.x + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f,
					LayoutToggleQuadButtonPos.y + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f
				);
				LayoutToggleQuadDrawList->AddImage(
					IconQuad->GetTextureSRV(),
					LayoutToggleQuadIconPos,
					ImVec2(LayoutToggleQuadIconPos.x + LayoutToggleIconSize, LayoutToggleQuadIconPos.y + LayoutToggleIconSize)
				);

				if (bLayoutToggleQuadClicked)
				{
					CurrentLayout = ELayout::Quad;
					ViewportManager.SetViewportLayout(EViewportLayout::Quad);
					ViewportManager.StartLayoutAnimation(true, ViewportIndex);
				}
			}
		}

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
		{
			// Single 레이아웃으로 전환 버튼
			if (IconSquare && IconSquare->GetTextureSRV())
			{
				const ImVec2 LayoutToggleSingleButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##LayoutToggleSingleButton", ImVec2(LayoutToggleButtonSize, LayoutToggleButtonSize));
				const bool bLayoutToggleSingleClicked = ImGui::IsItemClicked();
				const bool bLayoutToggleSingleHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* LayoutToggleSingleDrawList = ImGui::GetWindowDrawList();
				ImU32 LayoutToggleSingleBgColor = bLayoutToggleSingleHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					LayoutToggleSingleBgColor = IM_COL32(38, 38, 38, 255);
				}
				LayoutToggleSingleDrawList->AddRectFilled(LayoutToggleSingleButtonPos, ImVec2(LayoutToggleSingleButtonPos.x + LayoutToggleButtonSize, LayoutToggleSingleButtonPos.y + LayoutToggleButtonSize), LayoutToggleSingleBgColor, 4.0f);
				LayoutToggleSingleDrawList->AddRect(LayoutToggleSingleButtonPos, ImVec2(LayoutToggleSingleButtonPos.x + LayoutToggleButtonSize, LayoutToggleSingleButtonPos.y + LayoutToggleButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (중앙 정렬)
				const ImVec2 LayoutToggleSingleIconPos = ImVec2(
					LayoutToggleSingleButtonPos.x + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f,
					LayoutToggleSingleButtonPos.y + (LayoutToggleButtonSize - LayoutToggleIconSize) * 0.5f
				);
				LayoutToggleSingleDrawList->AddImage(
					IconSquare->GetTextureSRV(),
					LayoutToggleSingleIconPos,
					ImVec2(LayoutToggleSingleIconPos.x + LayoutToggleIconSize, LayoutToggleSingleIconPos.y + LayoutToggleIconSize)
				);

				if (bLayoutToggleSingleClicked)
				{
					CurrentLayout = ELayout::Single;
					ViewportManager.SetViewportLayout(EViewportLayout::Single);
					// 스플리터 비율을 저장하고 애니메이션 시작: Quad → Single
					ViewportManager.PersistSplitterRatios();
					ViewportManager.StartLayoutAnimation(false, ViewportIndex);
				}
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopID();
}

bool UViewportControlWidget::IsPilotModeActive(int32 ViewportIndex) const
{
	if (!GEditor)
	{
		return false;
	}

	UEditor* Editor = GEditor->GetEditorModule();
	if (!Editor || !Editor->IsPilotMode())
	{
		return false;
	}

	// 현재 뷰포트가 파일럿 모드를 사용 중인지 확인
	auto& ViewportManager = UViewportManager::GetInstance();
	return (ViewportManager.GetLastClickedViewportIndex() == ViewportIndex);
}

void UViewportControlWidget::RenderPilotModeExitButton(int32 ViewportIndex, ImVec2& InOutCursorPos)
{
	if (!GEditor)
	{
		return;
	}

	UEditor* Editor = GEditor->GetEditorModule();
	if (!Editor || !Editor->IsPilotMode())
	{
		return;
	}

	// △ 아이콘 버튼 (파일럿 모드 종료)
	constexpr float ExitButtonSize = 24.0f;
	constexpr float TriangleSize = 8.0f;

	ImVec2 ExitButtonPos = InOutCursorPos;
	ImGui::SetCursorScreenPos(ExitButtonPos);
	ImGui::InvisibleButton("##PilotModeExit", ImVec2(ExitButtonSize, ExitButtonSize));
	bool bExitClicked = ImGui::IsItemClicked();
	bool bExitHovered = ImGui::IsItemHovered();

	// 버튼 배경
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImU32 BgColor = bExitHovered ? IM_COL32(60, 40, 40, 255) : IM_COL32(40, 20, 20, 255);
	if (ImGui::IsItemActive())
	{
		BgColor = IM_COL32(80, 50, 50, 255);
	}
	DrawList->AddRectFilled(ExitButtonPos, ImVec2(ExitButtonPos.x + ExitButtonSize, ExitButtonPos.y + ExitButtonSize), BgColor, 4.0f);
	DrawList->AddRect(ExitButtonPos, ImVec2(ExitButtonPos.x + ExitButtonSize, ExitButtonPos.y + ExitButtonSize), IM_COL32(160, 80, 80, 255), 4.0f);

	// △ 아이콘 그리기 (Eject 느낌)
	ImVec2 Center = ImVec2(ExitButtonPos.x + ExitButtonSize * 0.5f, ExitButtonPos.y + ExitButtonSize * 0.5f);
	ImVec2 P1 = ImVec2(Center.x, Center.y - TriangleSize * 0.6f);
	ImVec2 P2 = ImVec2(Center.x - TriangleSize * 0.5f, Center.y + TriangleSize * 0.4f);
	ImVec2 P3 = ImVec2(Center.x + TriangleSize * 0.5f, Center.y + TriangleSize * 0.4f);
	DrawList->AddTriangleFilled(P1, P2, P3, IM_COL32(220, 180, 180, 255));

	if (bExitClicked)
	{
		Editor->RequestExitPilotMode();
		UE_LOG_INFO("ViewportControlWidget: Pilot mode exited via UI button");
	}

	if (bExitHovered)
	{
		ImGui::SetTooltip("Exit Pilot Mode (Alt + G)");
	}

	// 커서 위치 업데이트 (다음 버튼 배치용)
	InOutCursorPos.x += ExitButtonSize + 6.0f;
}
