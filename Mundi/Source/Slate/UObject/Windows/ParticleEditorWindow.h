#pragma once
#include "Source/Slate/Core/Windows/SWindow.h"

class ViewerState;
class UParticleModule;
class UParticleSystem;
class UParticleEmitter;
class UParticleModuleDetailWidget;
class AParticleSystemActor;
class UDistributionFloatBezier;
class UDistributionVectorBezier;

class SParticleEditorWindow : public SWindow
{
public:
	SParticleEditorWindow();
	virtual ~SParticleEditorWindow();

	bool Initialize(float StartX, float StartY, float Width, float Height, class UWorld* InWorld, ID3D11Device* InDevice);

	virtual void OnRender() override;
	virtual void OnUpdate(float DeltaSeconds) override;
	virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
	virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;
	virtual void OnMouseMove(FVector2D MousePos) override;

	void OnRenderViewport();

	bool IsOpen() const { return bIsOpen; }
	void Close() { bIsOpen = false; }

	void SetParticleSystem(UParticleSystem* InParticleSystem);

	// File operations
	void LoadParticleSystemFromFile();
	void SaveParticleSystemToFile();
	void SaveParticleSystemToFileAs();

private:
	// UI 렌더링
	void RenderTopToolbar();
	void RenderViewportPanel();
	void RenderDetailsPanel();
	void RenderEmittersPanel();
	void RenderCurveEditorPanel();
	void RenderStatsOverlay(const ImVec2& ViewportMin, const ImVec2& ViewportSize);
	void RenderBezierFloatCurveEditor(const char* PropName, UDistributionFloatBezier* BezierDist);
	void RenderBezierVectorCurveEditor(const char* PropName, UDistributionVectorBezier* BezierDist);

	// 헬퍼 함수
	UParticleModule* GetModuleFromCurrentEmitter(int32 ModuleIndex);
	void CreateTestParticleSystem();
	void ShowAddModuleContextMenu(int32 EmitterIndex);
	void AddNewEmitter();

	// Preview Viewport
	ViewerState* PreviewState = nullptr;
	FRect PreviewViewportRect = FRect(0, 0, 0, 0);
	ID3D11Device* Device = nullptr;

	// Preview Actor (파티클 미리보기용)
	AParticleSystemActor* PreviewActor = nullptr;

	// 프리뷰 업데이트 함수
	void UpdatePreviewActor();

	// State
	bool bIsOpen = true;
	bool bIsPlaying = false;
	bool bShowStatsOverlay = true;
	FString StatusMessage = "Ready";

	// 뷰포트 배경색
	FLinearColor ViewportBackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
	bool bShowBackgroundColorPicker = false;

	// Details Panel Widget
	UParticleModuleDetailWidget* DetailWidget = nullptr;

	// Selected Module
	UParticleModule* SelectedModule = nullptr;
	int32 SelectedModuleIndex = -1;

	// Particle System
	UParticleSystem* EditingParticleSystem = nullptr;

	// Current file path
	FWideString CurrentFilePath;

	// Selected Emitter
	int32 SelectedEmitterIndex = 0;

	// 현재 편집/미리보기 중인 LOD 레벨 인덱스 (0 = 최고 품질)
	int32 CurrentLODIndex = 0;

	// Event handlers
	void OnPlayClicked();
	void OnPauseClicked();
	void OnResetClicked();
	void OnRestartSimClicked();
	void OnRestartLevelClicked();

	// LOD 툴바 핸들러
	void OnRegenLOD();       // 모든 LOD 레벨 모듈 리스트 재빌드
	void OnAddLOD();         // 현재 LOD 아래에 새 LOD 레벨 추가
	void OnRemoveLOD();      // 현재 LOD 레벨 제거 (LOD 0은 제거 불가)
	void OnHigherLOD();      // 더 높은 품질(낮은 인덱스) LOD로 이동
	void OnLowerLOD();       // 더 낮은 품질(높은 인덱스) LOD로 이동
	void OnLowestLOD();      // 가장 낮은 품질 LOD로 이동

	// 현재 파티클 시스템의 최대 LOD 인덱스를 반환
	int32 GetMaxLODIndex() const;
};
