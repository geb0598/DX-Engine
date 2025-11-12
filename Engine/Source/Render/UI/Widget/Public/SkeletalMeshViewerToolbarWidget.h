#pragma once
#include "ViewportToolbarWidgetBase.h"
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
class USkeletalMeshViewerToolbarWidget : public UViewportToolbarWidgetBase
{
	DECLARE_CLASS(USkeletalMeshViewerToolbarWidget, UViewportToolbarWidgetBase)

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
	 * @brief Owning Window 설정 (그리드 설정 접근용)
	 */
	void SetOwningWindow(class USkeletalMeshViewerWindow* InOwningWindow) { OwningWindow = InOwningWindow; }

	/**
	 * @brief 현재 Gizmo Mode 가져오기
	 */
	EGizmoMode GetCurrentGizmoMode() const { return CurrentGizmoMode; }

	/**
	 * @brief Gizmo Mode 설정 (외부에서 변경 시 동기화용)
	 */
	void SetCurrentGizmoMode(EGizmoMode Mode) { CurrentGizmoMode = Mode; }

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

	/**
	 * @brief Scale Snap 활성화 여부
	 */
	bool IsScaleSnapEnabled() const { return bScaleSnapEnabled; }

	/**
	 * @brief Scale Snap 값
	 */
	float GetScaleSnapValue() const { return ScaleSnapValue; }

private:
	// ========================================
	// Rendering Functions
	// ========================================

	/**
	 * @brief Grid Settings 버튼 렌더링
	 */
	void RenderGridSettingsButton();

	/**
	 * @brief Camera Settings 팝업 내용 렌더링
	 */
	void RenderCameraSettingsPopupContent();

private:
	// ViewportClient (카메라 및 뷰 정보 접근용)
	FViewportClient* ViewportClient = nullptr;

	// Owning Window (그리드 설정 접근용)
	class USkeletalMeshViewerWindow* OwningWindow = nullptr;

	// Gizmo Mode 상태
	EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
	bool bSelectModeActive = false;

	// Rotation Snap 상태
	bool bRotationSnapEnabled = false;
	float RotationSnapAngle = 10.0f;

	// Scale Snap 상태
	bool bScaleSnapEnabled = false;
	float ScaleSnapValue = 1.0f;
};
