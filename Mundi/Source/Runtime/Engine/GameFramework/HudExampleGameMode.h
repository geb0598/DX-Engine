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
	// UI 업데이트
	// ────────────────────────────────────────────────

	/** 점수 UI 업데이트 */
	void UpdateScoreUI(int32 Score);

protected:
	// ────────────────────────────────────────────────
	// UI 위젯
	// ────────────────────────────────────────────────

	TSharedPtr<SButton> StartButton;
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

	/** 차량 정보 패널 배경 (왼쪽 하단) */
	TSharedPtr<SBorderBox> VehicleInfoBg;

	/** 차량 정보: 속도 텍스트 */
	TSharedPtr<STextBlock> VehicleSpeedText;

	/** 차량 정보: RPM 텍스트 */
	TSharedPtr<STextBlock> VehicleRpmText;

	/** 차량 정보: 기어 텍스트 */
	TSharedPtr<STextBlock> VehicleGearText;
};
