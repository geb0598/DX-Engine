#include "pch.h"
#include "Level/Public/GameInstance.h"
#include "Level/Public/World.h"
#include "Core/Public/AppWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/GameViewportClient.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"

IMPLEMENT_CLASS(UGameInstance, UEngineSubsystem)

UGameInstance::UGameInstance()
{
}

/**
 * @brief UEngineSubsystem 초기화
 */
void UGameInstance::Initialize()
{
	UEngineSubsystem::Initialize();
	UE_LOG_INFO("GameInstance: Subsystem initialized");
}

/**
 * @brief 게임 인스턴스 World 초기화 함수
 * StandAlone World와 Fullscreen Viewport 생성
 * @param InWindow 메인 윈도우
 * @param InScenePath 로드할 Scene 경로 (nullptr이면 빈 레벨 생성)
 */
void UGameInstance::InitializeWorld(FAppWindow* InWindow, const char* InScenePath)
{
	AppWindow = InWindow;

	// StandAlone World 생성 (PIE 타입)
	World = NewObject<UWorld>();
	World->SetWorldType(EWorldType::PIE);

	// GWorld 전역 포인터 설정 (Actor 초기화 시 필요)
	GWorld = World;

	// Scene 로드 또는 새 레벨 생성
	bool bLevelLoaded = false;
	if (InScenePath && strlen(InScenePath) > 0)
	{
		if (!World->LoadLevel(InScenePath))
		{
			UE_LOG_ERROR("GameInstance: Failed to load scene '%s', creating new level instead", InScenePath);
			World->CreateNewLevel();
		}
		else
		{
			UE_LOG_SUCCESS("GameInstance: Loaded scene '%s'", InScenePath);
			bLevelLoaded = true; // LoadLevel 내부에서 이미 BeginPlay 호출됨
		}
	}
	else
	{
		World->CreateNewLevel();
		UE_LOG_INFO("GameInstance: Created new level (no scene path provided)");
	}

	// LoadLevel이 성공하지 않은 경우에만 BeginPlay 호출 (LoadLevel 내부에서 이미 호출됨)
	if (!bLevelLoaded)
	{
		World->BeginPlay();
	}

	// Fullscreen Viewport 생성
	Viewport = new FViewport();
	ViewportClient = NewObject<UGameViewportClient>();

	// 윈도우 크기로 뷰포트 설정
	RECT ClientRect;
	GetClientRect(InWindow->GetWindowHandle(), &ClientRect);
	const uint32 Width = ClientRect.right - ClientRect.left;
	const uint32 Height = ClientRect.bottom - ClientRect.top;

	Viewport->SetSize(Width, Height);
	Viewport->SetInitialPosition(0, 0);
	// Note: FViewport는 FViewportClient*를 사용하지만,
	// StandAlone에서는 UGameViewportClient를 직접 관리
	ViewportClient->SetOwningViewport(Viewport);

	UE_LOG_SUCCESS("GameInstance: Initialized world with fullscreen viewport (%dx%d)", Width, Height);
}

/**
 * @brief UEngineSubsystem 종료 함수
 * World와 Viewport 리소스 정리
 */
void UGameInstance::Deinitialize()
{
	// World 정리
	if (World)
	{
		World->EndPlay();
		delete World;
		World = nullptr;
	}

	// Viewport 정리
	if (ViewportClient)
	{
		delete ViewportClient;
		ViewportClient = nullptr;
	}

	if (Viewport)
	{
		delete Viewport;
		Viewport = nullptr;
	}

	GWorld = nullptr;

	UEngineSubsystem::Deinitialize();
	UE_LOG_INFO("GameInstance: Deinitialize completed");
}

/**
 * @brief 매 프레임 업데이트
 * @param DeltaTime 프레임 시간
 */
void UGameInstance::Tick(float DeltaTime)
{
	// World Tick
	if (World)
	{
		World->Tick(DeltaTime);
	}

	// ViewportClient Tick
	if (ViewportClient)
	{
		ViewportClient->Tick();
	}
}

/**
 * @brief SceneView 생성
 * ViewportClient의 Transform 정보를 사용하여 렌더링용 SceneView 생성
 * @return 생성된 FSceneView 포인터 (호출자가 delete 필요)
 */
FSceneView* UGameInstance::CreateSceneView() const
{
	if (!ViewportClient || !Viewport || !World)
	{
		return nullptr;
	}

	// SceneView 생성
	FSceneView* SceneView = new FSceneView();

	// ViewportClient의 Transform으로 View/Projection 행렬 계산하여 초기화
	SceneView->InitializeWithMatrices(
		ViewportClient->GetViewMatrix(),
		ViewportClient->GetProjectionMatrix(),
		ViewportClient->GetViewLocation(),
		ViewportClient->GetViewRotation(),
		Viewport,
		World,
		EViewModeIndex::VMI_Gouraud,  // Default view mode
		ViewportClient->GetFOV(),
		ViewportClient->GetNearZ(),
		ViewportClient->GetFarZ()
	);

	return SceneView;
}
