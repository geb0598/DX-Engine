#pragma once
#include "Widget.h"
#include "Editor/Public/GizmoTypes.h"

class FViewportClient;
class UCamera;
class UTexture;

/**
 * @brief SkeletalMeshViewerWindow용 툴바 위젯
 *
 * Gizmo Mode 버튼(QWER), Rotation Snap, ViewType, Camera Settings, ViewMode 등을 포함합니다.
 * 메인 에디터의 ViewportControlWidget과 유사하지만 뷰어 윈도우에 특화되어 있습니다.
 */
class USkeletalMeshViewerToolbarWidget : public UWidget
{
	DECLARE_CLASS(USkeletalMeshViewerToolbarWidget, UWidget)

public:
	USkeletalMeshViewerToolbarWidget();
	virtual ~USkeletalMeshViewerToolbarWidget() override;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	/**
	 * @brief ViewportClient 설정 (카메라 및 뷰 정보 접근용)
	 */
	void SetViewportClient(FViewportClient* InViewportClient) { ViewportClient = InViewportClient; }

	/**
	 * @brief 현재 Gizmo Mode 가져오기
	 */
	EGizmoMode GetCurrentGizmoMode() const { return CurrentGizmoMode; }

	/**
	 * @brief Select Mode 활성화 여부
	 */
	bool IsSelectModeActive() const { return bSelectModeActive; }

	/**
	 * @brief Rotation Snap 활성화 여부
	 */
	bool IsRotationSnapEnabled() const { return bRotationSnapEnabled; }

	/**
	 * @brief Rotation Snap 각도
	 */
	float GetRotationSnapAngle() const { return RotationSnapAngle; }

private:
	// ========================================
	// Rendering Functions
	// ========================================

	/**
	 * @brief 뷰 아이콘 로드
	 */
	void LoadViewIcons();

	/**
	 * @brief 아이콘 버튼 렌더링 (공통 로직)
	 */
	bool DrawIconButton(const char* ID, UTexture* Icon, bool bActive, const char* Tooltip, float ButtonSize = 24.0f, float IconSize = 16.0f);

	/**
	 * @brief Gizmo Mode 버튼들 렌더링 (QWER)
	 */
	void RenderGizmoModeButtons();

	/**
	 * @brief Rotation Snap 컨트롤 렌더링
	 */
	void RenderRotationSnapControls();

	/**
	 * @brief View Type 버튼 렌더링 (Perspective/Orthographic)
	 */
	void RenderViewTypeButton();

	/**
	 * @brief Camera Speed 버튼 렌더링
	 */
	void RenderCameraSpeedButton();

	/**
	 * @brief View Mode 버튼 렌더링 (Lighting Mode)
	 */
	void RenderViewModeButton();

private:
	// ViewportClient (카메라 및 뷰 정보 접근용)
	FViewportClient* ViewportClient = nullptr;

	// View Type 아이콘들
	UTexture* IconPerspective = nullptr;
	UTexture* IconTop = nullptr;
	UTexture* IconBottom = nullptr;
	UTexture* IconLeft = nullptr;
	UTexture* IconRight = nullptr;
	UTexture* IconFront = nullptr;
	UTexture* IconBack = nullptr;

	// Gizmo Mode 아이콘들
	UTexture* IconSelect = nullptr;
	UTexture* IconTranslate = nullptr;
	UTexture* IconRotate = nullptr;
	UTexture* IconScale = nullptr;

	// Other 아이콘들
	UTexture* IconLitCube = nullptr;
	UTexture* IconCamera = nullptr;

	bool bIconsLoaded = false;

	// Gizmo Mode 상태
	EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
	bool bSelectModeActive = false;

	// Rotation Snap 상태
	bool bRotationSnapEnabled = false;
	float RotationSnapAngle = 45.0f;
};
