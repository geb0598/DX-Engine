#pragma once

#include "Editor/Public/Camera.h"

class FViewport;
class APlayerCameraManager;

class FViewportClient
{
public:
    FViewportClient();
    ~FViewportClient();

    // FutureEngine 철학: ViewportClient는 자신이 속한 Viewport를 알아야 함
    void SetOwningViewport(FViewport* InViewport) { OwningViewport = InViewport; }
    FViewport* GetOwningViewport() const { return OwningViewport; }

public:
    // ---------- 구성/질의 ----------
    void        SetViewType(EViewType InType);
    EViewType   GetViewType() const { return ViewType; }

    void        SetViewMode(EViewModeIndex InMode) { ViewMode = InMode; }
    EViewModeIndex GetViewMode() const { return ViewMode; }


    bool        IsOrtho() const { return ViewType != EViewType::Perspective; }




public:
    void Tick() const;
    void Draw(const FViewport* InViewport) const;


    static void MouseMove(FViewport* /*Viewport*/, int32 /*X*/, int32 /*Y*/) {}
    void CapturedMouseMove(FViewport* /*Viewport*/, int32 X, int32 Y)
    {
        LastDrag = { X, Y };
    }

    // ---------- 리사이즈/포커스 ----------
    void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }
    static void OnGainFocus() {}
    static void OnLoseFocus() {}
    static void OnClose() {}


    

    //FViewProjConstants GetViewportCameraData() const { return ViewportCamera->GetFViewProjConstants(); }

    UCamera* GetCamera()const { return ViewportCamera; }

    /**
     * Set player camera manager for PIE/Game mode
     * @param Manager Player camera manager, or nullptr to use editor camera
     */
    void SetPlayerCameraManager(APlayerCameraManager* Manager);

    /**
     * Prepare camera for rendering (update camera constants based on viewport)
     * Should be called before GetCameraConstants() each frame
     * @param InViewport D3D11 viewport information for aspect ratio and camera update
     */
    void PrepareCamera(const D3D11_VIEWPORT& InViewport);

    /**
     * Get camera constants for rendering
     * Uses player camera manager in PIE/Game mode, otherwise uses editor camera
     */
    const FCameraConstants& GetCameraConstants() const;

private:
    // 상태
    EViewType       ViewType = EViewType::Perspective;
    EViewModeIndex  ViewMode = EViewModeIndex::VMI_Gouraud;

    // 카메라 시스템 (타입별 분리)
    UCamera* ViewportCamera = nullptr;  // Editor camera
    APlayerCameraManager* PlayerCameraManager = nullptr;  // PIE/Game camera manager

    FViewport* OwningViewport = nullptr;  // FutureEngine: 소속 Viewport 참조
    
    // Store perspective camera state for restoration
    FVector SavedPerspectiveLocation = FVector(-15.0f, 0.f, 10.0f);
    FVector SavedPerspectiveRotation = FVector(0, 0, 0);
    float SavedPerspectiveFarZ = 1000.0f;
	


    // 뷰/입력 상태
    FPoint		ViewSize{ 0, 0 };
    FPoint		LastDrag{ 0, 0 };

    // Orthographic 뷰포트의 기준 높이 (픽셀 밀도 유지용)
    mutable float OrthoReferenceHeight = 0.0f;
};
