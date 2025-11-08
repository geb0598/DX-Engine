#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Editor/Public/GizmoTypes.h"

class FViewport;
class FViewportClient;
class UCamera;
class USkeletalMeshComponent;

/**
 * @brief SkeletalMesh 뷰어 윈도우
 * Editor 내부에서 복수의 SkeletalMesh 리소스를 View & Edit 할 수 있는 내부 Viewer
 *
 * Layout:
 * - 좌측: Skeleton Tree (본 계층 구조) [TODO]
 * - 중앙: 3D Viewport (독립 카메라) [TODO]
 * - 우측: Edit Tools (Transform, Gizmo 설정 등) [TODO]
 *
 * @note 현재는 기본 레이아웃 구조만 구현됨. 각 패널의 실제 기능은 추후 구현 예정
 *
 * TODO: SkeletalMeshComponent의 디테일 패널에서도 이 뷰어를 열 수 있는 기능 추가
 */
class USkeletalMeshViewerWindow : public UUIWindow
{
	DECLARE_CLASS(USkeletalMeshViewerWindow, UUIWindow);

public:
	USkeletalMeshViewerWindow();
	virtual ~USkeletalMeshViewerWindow() override;

	void Initialize() override;
	void Cleanup() override;

	void SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent) { SkeletalMeshComponent = InSkeletalMeshComponent; }

protected:
	void OnPreRenderWindow(float MenuBarOffset) override;
	void OnPostRenderWindow() override;
	/**
	 * @brief 3-패널 레이아웃을 렌더링하는 함수
	 * 좌측(25%): 스켈레톤 트리, 중앙(50%): 3D 뷰, 우측(25%): 편집 툴
	 */
	void RenderLayout();

	/**
	 * @brief 좌측 패널: Skeleton Tree 영역 렌더링 (Placeholder)
	 * TODO: 실제 본 계층 구조 트리 위젯 구현
	 */
	void RenderSkeletonTreePanel();

	/**
	 * @brief 중앙 패널: 3D Viewport 영역 렌더링 (Placeholder)
	 * TODO: 독립적인 카메라를 가진 3D 렌더링 뷰포트 구현
	 * TODO: 선택된 본의 Transform Gizmo 렌더링
	 */
	void Render3DViewportPanel();

	/**
	 * @brief 우측 패널: Edit Tools 영역 렌더링 (Placeholder)
	 * TODO: Transform 편집 UI, Gizmo 설정, 본 프로퍼티 등 구현
	 */
	void RenderEditToolsPanel();

	/**
	 * @brief 수직 Splitter (구분선) 렌더링 및 드래그 처리
	 * @param SplitterID 고유 ID
	 * @param Ratio 현재 비율 (0.0 ~ 1.0)
	 * @param MinRatio 최소 비율
	 * @param MaxRatio 최대 비율
	 * @param bInvertDirection true면 드래그 방향 반전 (우측 패널용)
	 */
	void RenderVerticalSplitter(const char* SplitterID, float& Ratio, float MinRatio, float MaxRatio, bool bInvertDirection = false);

	/**
	 * @brief 뷰포트 메뉴바 렌더링
	 */
	void RenderViewportMenuBar();

	/**
	 * @brief 카메라 컨트롤 UI 렌더링
	 * @param InCamera 카메라 객체
	 */
	void RenderCameraControls(UCamera& InCamera);

	/**
	 * @brief 뷰 아이콘 로드
	 */
	void LoadViewIcons();

private:
	// 패널 크기 비율 (드래그 가능)
	float LeftPanelWidthRatio = 0.25f;
	float RightPanelWidthRatio = 0.25f;

	// Splitter 설정
	static constexpr float SplitterWidth = 4.0f;
	static constexpr float MinPanelRatio = 0.1f;
	static constexpr float MaxPanelRatio = 0.8f;

	// 독립적인 뷰포트 및 카메라
	FViewport* ViewerViewport = nullptr;
	FViewportClient* ViewerViewportClient = nullptr;

	// 독립적인 렌더 타겟
	ID3D11Texture2D* ViewerRenderTargetTexture = nullptr;
	ID3D11RenderTargetView* ViewerRenderTargetView = nullptr;
	ID3D11ShaderResourceView* ViewerShaderResourceView = nullptr;
	ID3D11DepthStencilView* ViewerDepthStencilView = nullptr;
	ID3D11Texture2D* ViewerDepthStencilTexture = nullptr;

	uint32 ViewerWidth = 800;
	uint32 ViewerHeight = 600;

	// 초기화 및 정리 상태 플래그
	bool bIsInitialized = false;
	bool bIsCleanedUp = false;

	// 렌더링할 SkeletalMeshComponent
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

	// 독립적인 BatchLines (Grid 렌더링용)
	class UBatchLines* ViewerBatchLines = nullptr;

	// View Type 아이콘들
	class UTexture* IconPerspective = nullptr;
	class UTexture* IconTop = nullptr;
	class UTexture* IconBottom = nullptr;
	class UTexture* IconLeft = nullptr;
	class UTexture* IconRight = nullptr;
	class UTexture* IconFront = nullptr;
	class UTexture* IconBack = nullptr;

	// Gizmo Mode 아이콘들
	class UTexture* IconSelect = nullptr;
	class UTexture* IconTranslate = nullptr;
	class UTexture* IconRotate = nullptr;
	class UTexture* IconScale = nullptr;

	// Other 아이콘들
	class UTexture* IconLitCube = nullptr;
	class UTexture* IconCamera = nullptr;

	bool bIconsLoaded = false;

	// Gizmo Mode 상태
	EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
	bool bSelectModeActive = false; // Q 버튼 - Select 모드

	// Rotation Snap 상태
	bool bRotationSnapEnabled = false;
	float RotationSnapAngle = 45.0f;

	/**
	 * @brief 렌더 타겟 생성
	 */
	void CreateRenderTarget(uint32 Width, uint32 Height);

	/**
	 * @brief 렌더 타겟 해제
	 */
	void ReleaseRenderTarget();
};
