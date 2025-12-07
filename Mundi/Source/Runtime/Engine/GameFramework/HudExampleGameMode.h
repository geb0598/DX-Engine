// ────────────────────────────────────────────────────────────────────────────
// HudExampleGameMode.h
// SGameHUD 사용 예제 게임 모드
// ────────────────────────────────────────────────────────────────────────────
#pragma once

#include "GameModeBase.h"
#include "Source/Runtime/Core/Memory/PointerTypes.h"
#include "AHudExampleGameMode.generated.h"

class SButton;
class STextBlock;
class SImage;
class SGradientBox;
class SBorderBox;
class SMinimap;
class SPanel;

// 전방 선언
class ACameraActor;

/**
 * 게임 상태
 */
enum class EGameState
{
	MainMenu,              // 메인 메뉴 (시작 화면)
	Tutorial_Camera,       // 튜토리얼: 카메라 연출
	Tutorial_Comic,        // 튜토리얼: 만화/컷씬
	Playing,               // 게임 플레이 중
};

/**
 * AHudExampleGameMode
 *
 * SGameHUD를 사용하여 게임 UI를 구성하는 예제 게임 모드입니다.
 */
UCLASS(DisplayName="HUD 예제 게임 모드", Description="SGameHUD 사용 예제 게임 모드입니다.")
class AHudExampleGameMode : public AGameModeBase
{
public:
	GENERATED_REFLECTION_BODY()

	AHudExampleGameMode();

protected:
	~AHudExampleGameMode() override = default;

public:
	// ────────────────────────────────────────────────
	// 생명주기
	// ────────────────────────────────────────────────

	void BeginPlay() override;
	void EndPlay() override;
	void Tick(float DeltaSeconds) override;

	// ────────────────────────────────────────────────
	// 게임 라이프사이클
	// ────────────────────────────────────────────────

	void StartGame() override;

	// ────────────────────────────────────────────────
	// 게임 상태 관리
	// ────────────────────────────────────────────────

	/** 현재 게임 상태 */
	EGameState GetGameState() const { return CurrentGameState; }

	/** 메인 메뉴로 전환 */
	void ShowMainMenu();

	/** 게임 플레이 시작 */
	void StartGamePlay();

	/** 카메라 시네마틱 시작 */
	void StartCameraCinematic();

	/** 튜토리얼 만화/컷씬 표시 */
	void ShowTutorialComic();

	// ────────────────────────────────────────────────
	// UI 업데이트
	// ────────────────────────────────────────────────

	/** 점수 UI 업데이트 */
	void UpdateScoreUI(int32 Score);

protected:
	// ────────────────────────────────────────────────
	// 게임 상태
	// ────────────────────────────────────────────────

	/** 현재 게임 상태 */
	EGameState CurrentGameState = EGameState::MainMenu;

	// ────────────────────────────────────────────────
	// UI 위젯 - 메인 메뉴
	// ────────────────────────────────────────────────

	/** 타이틀 이미지 */
	TSharedPtr<SImage> TitleImage;

	/** "Press Space to Continue" 배경 그라데이션 (메인 메뉴용) */
	TSharedPtr<SGradientBox> PressSpaceBg;

	/** "Press Space to Continue" 이미지 (메인 메뉴용) */
	TSharedPtr<SImage> PressSpaceImage;

	/** "Press Any Key" 배경 그라데이션 (튜토리얼 카메라용) */
	TSharedPtr<SGradientBox> PressSpaceBg2;

	/** "Press Any Key" 이미지 (튜토리얼 카메라용) */
	TSharedPtr<SImage> PressSpaceImage2;

	/** 메인 메뉴 타이머 */
	float MainMenuTimer = 0.f;

	/** 최소 대기 시간 (애니메이션 완료 전에는 입력 무시) */
	float MinWaitTime = 3.5f;  // 애니메이션 시간과 동일

	/** 입력 대기 준비 완료 */
	bool bReadyForInput = false;

	/** 초기 카메라 설정 완료 플래그 */
	bool bInitialCameraSet = false;

	/** 튜토리얼 카메라 타이머 */
	float TutorialCameraTimer = 0.f;

	/** 튜토리얼 카메라 대기 시간 (조작법 보여주는 시간) */
	float TutorialCameraWaitTime = 5.0f;

	/** 튜토리얼 카메라 입력 대기 준비 */
	bool bTutorialCameraReady = false;

	/** 만화 현재 장면 인덱스 (0~7) */
	int32 CurrentComicIndex = 0;

	/** 만화 장면 타이머 */
	float ComicSceneTimer = 0.f;

	/** 만화 각 장면 대기 시간 */
	float ComicSceneWaitTime = 4.0f;

	/** 만화 총 장면 수 */
	static constexpr int32 TotalComicScenes = 7;

	/** 만화 시작 후 입력 무시 시간 (키 중복 방지) */
	float ComicInputIgnoreTime = 0.5f;

	/** 입력 무시 시간 이후 키 상태 초기화 완료 여부 */
	bool bComicInputReady = false;

	/** 이전 프레임 키 상태 (키 중복 방지) */
	bool bPrevSpacePressed = false;
	bool bPrevEnterPressed = false;
	bool bPrevEscPressed = false;

	// ────────────────────────────────────────────────
	// 카메라 연출
	// ────────────────────────────────────────────────

	/** 타이틀 화면 카메라 (게임 시작 시) */
	ACameraActor* TitleCamera = nullptr;

	/** 튜토리얼 카메라 (입력 후 전환) */
	ACameraActor* TutorialCamera = nullptr;

	/** 카메라 전환 블렌드 시간 */
	float CameraBlendTime = 1.0f;

	TSharedPtr<SButton> StartButton;

	// ────────────────────────────────────────────────
	// UI 위젯 - 게임 플레이
	// ────────────────────────────────────────────────
	TSharedPtr<STextBlock> ScoreText;
	TSharedPtr<STextBlock> SpeedText;
	TSharedPtr<STextBlock> RpmText;
	TSharedPtr<STextBlock> GearText;

	/** 박스 아이콘 */
	TSharedPtr<SImage> BoxesIcon;

	/** "x BOXES LEFT" 텍스트 이미지 */
	TSharedPtr<SImage> BoxesLeftText;

	/** 박스 개수 텍스트 */
	TSharedPtr<STextBlock> BoxesCountText;

	/** 테스트용 이미지 위젯 */
	TSharedPtr<SImage> TestImage;

	/** "REACH HOME" 배경 그라데이션 */
	TSharedPtr<SGradientBox> ReachHomeBg;

	/** "REACH HOME" 텍스트 이미지 (상단 중앙) */
	TSharedPtr<SImage> ReachHomeText;

	/** 차량 정보 패널 (속도, RPM, 기어를 담는 컨테이너) */
	TSharedPtr<SPanel> VehicleInfoPanel;

	/** 차량 정보: 속도 텍스트 (카트 이미지 위) */
	TSharedPtr<STextBlock> VehicleSpeedText;

	/** 차량 정보: RPM 텍스트 */
	TSharedPtr<STextBlock> VehicleRpmText;

	/** 차량 정보: 기어 텍스트 */
	TSharedPtr<STextBlock> VehicleGearText;

	/** 미니맵 */
	TSharedPtr<SMinimap> Minimap;

	/** 카트 배경 이미지 (좌하단) */
	TSharedPtr<SImage> CartImage;

	// ────────────────────────────────────────────────
	// UI 위젯 - 튜토리얼 만화/컷씬
	// ────────────────────────────────────────────────

	/** 만화/컷씬 전체 화면 이미지 (8장) */
	TArray<TSharedPtr<SImage>> ComicImages;
};
