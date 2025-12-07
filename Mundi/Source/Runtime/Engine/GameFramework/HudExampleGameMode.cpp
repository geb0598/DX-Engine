// ────────────────────────────────────────────────────────────────────────────
// HudExampleGameMode.cpp
// SGameHUD 사용 예제 게임 모드 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "HudExampleGameMode.h"
#include "DancingCharacter.h"
#include "Vehicle.h"

#include "GameUI/SGameHUD.h"
#include "GameUI/SButton.h"
#include "GameUI/STextBlock.h"
#include "GameUI/SImage.h"
#include "GameUI/SGradientBox.h"
#include "GameUI/SBorderBox.h"
#include "GameUI/SMinimap.h"
#include "PlayerController.h"
#include "CargoComponent.h"

// ────────────────────────────────────────────────────────────────────────────
// 생성자
// ────────────────────────────────────────────────────────────────────────────

AHudExampleGameMode::AHudExampleGameMode()
{
	// DefaultPawnClass = ADancingCharacter::StaticClass();
	DefaultPawnClass = AVehicle::StaticClass();
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!SGameHUD::Get().IsInitialized())
		return;

	// 이미 위젯이 생성되어 있으면 중복 생성 방지
	if (VehicleSpeedText)
		return;

	// 시작 버튼 (화면 중앙)
	/*StartButton = MakeShared<SButton>();
	StartButton->SetText(L"게임 시작")
		.SetBackgroundColor(FSlateColor(0.2f, 0.5f, 0.8f, 1.f))
		.SetFontSize(24.f)
		.SetCornerRadius(8.f)
		.OnClicked([this]() {
			StartGame();
		});

	SGameHUD::Get().AddWidget(StartButton)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, 0.f)
		.SetSize(200.f, 60.f);*/

	// 점수 텍스트 (좌상단)
	ScoreText = MakeShared<STextBlock>();
	ScoreText->SetText(L"점수: 0")
		.SetFontSize(32.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

	// 테스트용 이미지 (화면 좌상단)
	TestImage = MakeShared<SImage>();
	TestImage->SetTexture(L"Data/Textures/Dumb/DumbRider.png");

	SGameHUD::Get().AddWidget(TestImage)
		.SetAnchor(0.0f, 0.0f)
		.SetOffset(30.f, 0.f)
		.SetSize(250.f, 250.f);

	// Fade In + Scale 애니메이션 테스트 (더 느리게, 더 명확하게)
	//TestImage->SetOpacity(0.0f);  // 완전히 투명하게 시작
	//TestImage->SetScale(FVector2D(0.3f, 0.3f));  // 아주 작게 시작
	//TestImage->PlayFadeIn(2.0f, EEasingType::EaseOutCubic);  // 2초 동안 Fade In
	//TestImage->PlayScaleAnimation(FVector2D(1.0f, 1.0f), 2.0f, EEasingType::EaseOutCubic);  // 2초 동안 확대

	SGameHUD::Get().AddWidget(ScoreText)
		.SetAnchor(0.f, 0.f)
		.SetOffset(20.f, 20.f)
		.SetSize(300.f, 50.f);

	// 우하단 속도/기어/RPM UI는 제거됨 (왼쪽 하단 차량 정보 패널로 대체)

	// 박스 아이콘
	BoxesIcon = MakeShared<SImage>();
	BoxesIcon->SetTexture(L"Data/Textures/Dumb/Box.png");

	SGameHUD::Get().AddWidget(BoxesIcon)
		.SetAnchor(1.0f, 1.0f)  // 우하단
		.SetPivot(1.0f, 1.0f)   // 우하단 기준
		.SetOffset(-250.f, -80.f)  // 우하단에서 왼쪽, 위로 오프셋
		.SetSize(80.f, 80.f);

	// "x BOXES LEFT" 텍스트
	BoxesLeftText = MakeShared<SImage>();
	BoxesLeftText->SetTexture(L"Data/Textures/Dumb/Boxesleft.png");

	SGameHUD::Get().AddWidget(BoxesLeftText)
		.SetAnchor(1.0f, 1.0f)  // 우하단
		.SetPivot(1.0f, 1.0f)   // 우하단 기준
		.SetOffset(-120.f, -100.f)  // 박스 아이콘 오른쪽
		.SetSize(150.f, 40.f);

	// 박스 개수 텍스트 (박스 아이콘 중앙에서 약간 오른쪽 아래)
	BoxesCountText = MakeShared<STextBlock>();
	BoxesCountText->SetText(L"0")
		.SetFontSize(27.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Center)
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(BoxesCountText)
		.SetAnchor(1.0f, 1.0f)
		.SetPivot(1.0f, 1.0f)  // 우하단 기준 (박스 아이콘과 동일)
		.SetOffset(-225.f, -65.f)  // 박스 아이콘에서 오른쪽(+40), 아래(+40) 이동
		.SetSize(80.f, 80.f);  // 박스 아이콘과 동일한 크기

	// "REACH HOME" 배경 그라데이션 (상단 중앙)
	ReachHomeBg = MakeShared<SGradientBox>();
	ReachHomeBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetFadeWidth(150.f);

	SGameHUD::Get().AddWidget(ReachHomeBg)
		.SetAnchor(0.5f, 0.0f)  // 상단 중앙
		.SetPivot(0.5f, 0.0f)   // 상단 중앙 기준
		.SetOffset(0.f, 15.f)   // 위에서 15픽셀 아래
		.SetSize(400, 40.f);  // 텍스트보다 크게

	// "REACH HOME" 텍스트 (상단 중앙, 배경 중앙에 배치)
	ReachHomeText = MakeShared<SImage>();
	ReachHomeText->SetTexture(L"Data/Textures/Dumb/ReachHome.png");

	SGameHUD::Get().AddWidget(ReachHomeText)
		.SetAnchor(0.5f, 0.0f)  // 상단 중앙
		.SetPivot(0.5f, 0.5f)   // 이미지 중앙을 기준점으로
		.SetOffset(0.f, 36.f)   // 배경 중앙에 위치 (15 + 40/2 = 40)
		.SetSize(160.f, 30.f);  // 크기 증가

	// ─────────────────────────────────────────────────
	// 차량 정보 패널 (왼쪽 하단)
	// ─────────────────────────────────────────────────

	// 배경 박스
	VehicleInfoBg = MakeShared<SBorderBox>();
	VehicleInfoBg->SetBackgroundColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.7f))  // 반투명 검정
		.SetBorderColor(FSlateColor(1.0f, 1.0f, 1.0f, 0.4f))  // 반투명 흰색 테두리
		.SetBorderThickness(2.0f)
		.SetCornerRadius(8.0f);

	SGameHUD::Get().AddWidget(VehicleInfoBg)
		.SetAnchor(0.0f, 1.0f)  // 왼쪽 하단
		.SetPivot(0.0f, 1.0f)   // 왼쪽 하단 기준
		.SetOffset(20.f, -20.f)  // 왼쪽, 아래에서 20픽셀 안쪽
		.SetSize(200.f, 100.f);

	// 속도 텍스트
	VehicleSpeedText = MakeShared<STextBlock>();
	VehicleSpeedText->SetText(L"0 km/h")
		.SetFontSize(28.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

	SGameHUD::Get().AddWidget(VehicleSpeedText)
		.SetAnchor(0.0f, 1.0f)
		.SetPivot(0.0f, 1.0f)
		.SetOffset(35.f, -80.f)  // 배경 안쪽, 위쪽
		.SetSize(180.f, 35.f);

	// RPM 텍스트
	VehicleRpmText = MakeShared<STextBlock>();
	VehicleRpmText->SetText(L"0 RPM")
		.SetFontSize(18.f)
		.SetColor(FSlateColor(0.8f, 0.8f, 0.8f, 1.f))  // 밝은 회색
		.SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black());

	SGameHUD::Get().AddWidget(VehicleRpmText)
		.SetAnchor(0.0f, 1.0f)
		.SetPivot(0.0f, 1.0f)
		.SetOffset(35.f, -60.f)  // 속도 아래
		.SetSize(180.f, 25.f);

	// 기어 텍스트
	VehicleGearText = MakeShared<STextBlock>();
	VehicleGearText->SetText(L"GEAR: N")
		.SetFontSize(20.f)
		.SetColor(FSlateColor(1.f, 0.8f, 0.2f, 1.f))  // 노란색
		.SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black());

	SGameHUD::Get().AddWidget(VehicleGearText)
		.SetAnchor(0.0f, 1.0f)
		.SetPivot(0.0f, 1.0f)
		.SetOffset(35.f, -30.f)  // RPM 아래
		.SetSize(180.f, 25.f);

	// ─────────────────────────────────────────────────
	// 미니맵 (왼쪽 하단, 차량 정보 패널 위)
	// ─────────────────────────────────────────────────

	Minimap = MakeShared<SMinimap>();
	Minimap->SetMapTexture(L"Data/Textures/Dumb/Minimap.png")  // 미니맵 이미지 (준비 필요)
		.SetPlayerMarkerTexture(L"Data/Textures/Dumb/PlayerMarker.png")  // 플레이어 마커 (준비 필요)
		.SetWorldBounds(FVector(-278.5f, -116.5f, 0.f), FVector(278.5f, 116.5f, 0.f))  // 실제 맵 크기 (X: 557, Y: 233)
		.SetMarkerSize(12.f)
		.SetZoomLevel(150.f);  // 플레이어 주변 150 유닛 반경 표시 (작을수록 확대됨)

	SGameHUD::Get().AddWidget(Minimap)
		.SetAnchor(0.0f, 1.0f)  // 왼쪽 하단
		.SetPivot(0.0f, 1.0f)   // 왼쪽 하단 기준
		.SetOffset(20.f, -140.f)  // 차량 정보 패널 위 (20 + 100 + 20 = 140)
		.SetSize(180.f, 180.f);   // 정사각형 미니맵
}

void AHudExampleGameMode::EndPlay()
{
	Super::EndPlay();

	// HUD 위젯 정리
	if (SGameHUD::Get().IsInitialized())
	{
		if (StartButton)
		{
			SGameHUD::Get().RemoveWidget(StartButton);
			StartButton.Reset();
		}
		if (ScoreText)
		{
			SGameHUD::Get().RemoveWidget(ScoreText);
			ScoreText.Reset();
		}
		if (BoxesIcon)
		{
			SGameHUD::Get().RemoveWidget(BoxesIcon);
			BoxesIcon.Reset();
		}
		if (BoxesLeftText)
		{
			SGameHUD::Get().RemoveWidget(BoxesLeftText);
			BoxesLeftText.Reset();
		}
		if (BoxesCountText)
		{
			SGameHUD::Get().RemoveWidget(BoxesCountText);
			BoxesCountText.Reset();
		}
		if (ReachHomeBg)
		{
			SGameHUD::Get().RemoveWidget(ReachHomeBg);
			ReachHomeBg.Reset();
		}
		if (ReachHomeText)
		{
			SGameHUD::Get().RemoveWidget(ReachHomeText);
			ReachHomeText.Reset();
		}
		if (VehicleInfoBg)
		{
			SGameHUD::Get().RemoveWidget(VehicleInfoBg);
			VehicleInfoBg.Reset();
		}
		if (VehicleSpeedText)
		{
			SGameHUD::Get().RemoveWidget(VehicleSpeedText);
			VehicleSpeedText.Reset();
		}
		if (VehicleRpmText)
		{
			SGameHUD::Get().RemoveWidget(VehicleRpmText);
			VehicleRpmText.Reset();
		}
		if (VehicleGearText)
		{
			SGameHUD::Get().RemoveWidget(VehicleGearText);
			VehicleGearText.Reset();
		}
		if (Minimap)
		{
			SGameHUD::Get().RemoveWidget(Minimap);
			Minimap.Reset();
		}
		if (BoxesIcon)
		{
			SGameHUD::Get().RemoveWidget(BoxesIcon);
			BoxesIcon.Reset();
		}
		if (BoxesLeftText)
		{
			SGameHUD::Get().RemoveWidget(BoxesLeftText);
			BoxesLeftText.Reset();
		}
		if (BoxesCountText)
		{
			SGameHUD::Get().RemoveWidget(BoxesCountText);
			BoxesCountText.Reset();
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 라이프사이클
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::StartGame()
{
	Super::StartGame();

	// 게임 시작 시 시작 버튼 숨기기
	if (StartButton)
	{
		StartButton->SetVisibility(ESlateVisibility::Hidden);
	}
}

// ────────────────────────────────────────────────────────────────────────────
// UI 업데이트
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::UpdateScoreUI(int32 Score)
{
	if (ScoreText)
	{
		ScoreText->SetText(L"점수: " + std::to_wstring(Score));
	}
}

void AHudExampleGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GetPlayerController()) return;

	// 현재 플레이어의 Pawn이 Vehicle인지 확인
	APawn* PlayerPawn = GetPlayerController()->GetPawn();
	if (!PlayerPawn) return;

	AVehicle* Vehicle = Cast<AVehicle>(PlayerPawn);
	if (!Vehicle || !Vehicle->VehicleMovement) return;

	// 우하단 속도/기어/RPM UI는 제거됨

	// 박스 개수 UI 업데이트
	if (BoxesCountText)
	{
		UCargoComponent* CargoComp = Cast<UCargoComponent>(Vehicle->GetComponent(UCargoComponent::StaticClass()));
		if (CargoComp)
		{
			int32 BoxCount = CargoComp->GetValidCargoCount();
			BoxesCountText->SetText(std::to_wstring(BoxCount));
		}
	}

	// ─────────────────────────────────────────────────
	// 차량 정보 패널 업데이트 (왼쪽 하단)
	// ─────────────────────────────────────────────────

	// 속도 업데이트
	if (VehicleSpeedText)
	{
		float SpeedMs = Vehicle->VehicleMovement->GetForwardSpeed();
		float SpeedKmh = std::abs(SpeedMs) * 3.6f;
		int32 SpeedInt = static_cast<int32>(SpeedKmh);
		VehicleSpeedText->SetText(std::to_wstring(SpeedInt) + L" km/h");
	}

	// RPM 업데이트
	if (VehicleRpmText)
	{
		float Rpm = Vehicle->VehicleMovement->GetEngineRPM();
		int32 RpmInt = static_cast<int32>(Rpm);
		VehicleRpmText->SetText(std::to_wstring(RpmInt) + L" RPM");
	}

	// 기어 업데이트
	if (VehicleGearText)
	{
		int32 Gear = Vehicle->VehicleMovement->GetCurrentGear();
		std::wstring GearStr;
		if (Gear < 0)
			GearStr = L"GEAR: R";
		else if (Gear == 0)
			GearStr = L"GEAR: N";
		else
			GearStr = L"GEAR: " + std::to_wstring(Gear);

		VehicleGearText->SetText(GearStr);
	}

	// ─────────────────────────────────────────────────
	// 미니맵 업데이트
	// ─────────────────────────────────────────────────

	if (Minimap)
	{
		// 플레이어 위치 업데이트
		FVector PlayerPos = Vehicle->GetActorLocation();
		Minimap->UpdatePlayerPosition(PlayerPos);

		// 플레이어 회전 업데이트 (Quaternion을 Euler로 변환)
		FQuat PlayerQuat = Vehicle->GetActorRotation();
		FVector EulerAngles = PlayerQuat.ToEulerZYXDeg();
		float PlayerYaw = EulerAngles.Z;  // Yaw는 Z축 회전
		Minimap->UpdatePlayerRotation(PlayerYaw);
	}

	// 박스 개수 UI 업데이트
	if (BoxesCountText)
	{
		UCargoComponent* CargoComp = Cast<UCargoComponent>(Vehicle->GetComponent(UCargoComponent::StaticClass()));
		if (CargoComp)
		{
			int32 BoxCount = CargoComp->GetValidCargoCount();
			BoxesCountText->SetText(std::to_wstring(BoxCount));
		}
	}
}
