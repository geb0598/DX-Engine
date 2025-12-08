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
#include "GameUI/SPanel.h"
#include "PlayerController.h"
#include "CargoComponent.h"
#include "CameraActor.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"

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

	// ─────────────────────────────────────────────────
	// 메인 메뉴 UI
	// ─────────────────────────────────────────────────

	// 타이틀 이미지 (화면 중앙, Fade In + Scale 애니메이션)
	TitleImage = MakeShared<SImage>();
	TitleImage->SetTexture(L"Data/Textures/Dumb/DumbRider3d.png")
		.SetHighQualityInterpolation(true);  // 고품질 보간 적용

	SGameHUD::Get().AddWidget(TitleImage)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 이미지 중앙 기준
		.SetOffset(0.f, -10.f)  // 살짝 위로
		.SetSize(600.f, 300.f);  // 타이틀 크기 (조정 필요)

	// 애니메이션: Scale만 (Fade In 제거)
	TitleImage->SetScale(FVector2D(0.7f, 0.7f));  // 70% 크기로 시작

	// Scale 애니메이션만 실행 (3.5초)
	TitleImage->PlayScaleAnimation(FVector2D(1.0f, 1.0f), 3.5f, EEasingType::EaseOutCubic);

	// "Press Space to Continue" 배경 그라데이션 (타이틀 오른쪽 아래)
	PressSpaceBg = MakeShared<SGradientBox>();
	PressSpaceBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetFadeWidth(80.f);

	SGameHUD::Get().AddWidget(PressSpaceBg)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙 기준
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, 200.f)  // 타이틀 오른쪽 아래 (더 아래로: 160 → 200)
		.SetSize(280.f, 28.f);    // 두께 감소 (40 → 30)

	// "Press Space to Continue" 이미지 (메인 메뉴용)
	PressSpaceImage = MakeShared<SImage>();
	PressSpaceImage->SetTexture(L"Data/Textures/Dumb/pressanykey.png");

	SGameHUD::Get().AddWidget(PressSpaceImage)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙 기준
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, 201.f)  // 배경과 같은 위치
		.SetSize(180.f, 20.f);    // 배경과 같은 크기

	// 초기에는 숨김 (애니메이션 완료 후 표시)
	PressSpaceBg->SetVisibility(ESlateVisibility::Hidden);
	PressSpaceImage->SetVisibility(ESlateVisibility::Hidden);

	// "Press Any Key" 배경 그라데이션 (튜토리얼 카메라용 - 위치 조정됨)
	PressSpaceBg2 = MakeShared<SGradientBox>();
	PressSpaceBg2->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetFadeWidth(80.f);

	SGameHUD::Get().AddWidget(PressSpaceBg2)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙 기준
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(300.f, 350.f)  // 오른쪽(+50), 아래(+250)로 조정
		.SetSize(280.f, 28.f);

	// "Press Any Key" 이미지 (튜토리얼 카메라용 - 위치 조정됨)
	PressSpaceImage2 = MakeShared<SImage>();
	PressSpaceImage2->SetTexture(L"Data/Textures/Dumb/pressanykey.png");

	SGameHUD::Get().AddWidget(PressSpaceImage2)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙 기준
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(300.f, 351.f)  // 배경과 같은 위치
		.SetSize(180.f, 20.f);

	PressSpaceBg2->SetVisibility(ESlateVisibility::Hidden);
	PressSpaceImage2->SetVisibility(ESlateVisibility::Hidden);

	// 시작 버튼 (화면 중앙) - 일단 숨김 처리 (자동 전환 방식 사용)
	StartButton = MakeShared<SButton>();
	StartButton->SetText(L"게임 시작")
		.SetBackgroundColor(FSlateColor(0.2f, 0.5f, 0.8f, 1.f))
		.SetFontSize(24.f)
		.SetCornerRadius(8.f)
		.OnClicked([this]() {
			StartGamePlay();
		});

	SGameHUD::Get().AddWidget(StartButton)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, 100.f)  // 타이틀 아래
		.SetSize(200.f, 60.f);

	StartButton->SetVisibility(ESlateVisibility::Hidden);  // 자동 전환 사용하므로 숨김

	// ─────────────────────────────────────────────────
	// 게임 플레이 UI (성능 테스트를 위해 일부 주석 처리 가능)
	// ─────────────────────────────────────────────────

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

	// 박스 아이콘 (왼쪽 하단으로 이동)
	BoxesIcon = MakeShared<SImage>();
	BoxesIcon->SetTexture(L"Data/Textures/Dumb/Box.png");

	SGameHUD::Get().AddWidget(BoxesIcon)
		.SetAnchor(0.0f, 1.0f)  // 좌하단
		.SetPivot(0.0f, 1.0f)   // 좌하단 기준
		.SetOffset(20.f, -300.f)  // 미니맵, 차량 정보 위
		.SetSize(80.f, 80.f);

	// "x BOXES LEFT" 텍스트
	BoxesLeftText = MakeShared<SImage>();
	BoxesLeftText->SetTexture(L"Data/Textures/Dumb/Boxesleft.png");

	SGameHUD::Get().AddWidget(BoxesLeftText)
		.SetAnchor(0.0f, 1.0f)  // 좌하단
		.SetPivot(0.0f, 1.0f)   // 좌하단 기준
		.SetOffset(110.f, -320.f)  // 박스 아이콘 오른쪽
		.SetSize(150.f, 40.f);

	// 박스 개수 텍스트 (박스 아이콘 중앙)
	BoxesCountText = MakeShared<STextBlock>();
	BoxesCountText->SetText(L"0")
		.SetFontSize(27.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Center)
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(BoxesCountText)
		.SetAnchor(0.0f, 1.0f)  // 좌하단
		.SetPivot(0.0f, 1.0f)   // 좌하단 기준
		.SetOffset(45.f, -285.f)  // 박스 아이콘 중앙
		.SetSize(80.f, 80.f);

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
	// 카트 이미지 (좌하단) - 텍스트보다 먼저 그려야 텍스트가 위에 보임
	// ─────────────────────────────────────────────────

	CartImage = MakeShared<SImage>();
	CartImage->SetTexture(L"Data/Textures/Dumb/CartPanel.png")
		.SetRotation(-10.f)  // 왼쪽으로 10도 회전
		.SetHighQualityInterpolation(true);  // 고품질 보간으로 앤티앨리어싱 개선

	SGameHUD::Get().AddWidget(CartImage)
		.SetAnchor(0.0f, 1.0f)  // 좌하단
		.SetPivot(0.0f, 1.0f)   // 좌하단 기준
		.SetOffset(-50.f, -20.f)  // 회전으로 인한 공백 보정 (왼쪽으로 더 밀기)
		.SetSize(400.f, 300.f);   // 카트 이미지 크기

	// ─────────────────────────────────────────────────
	// 차량 정보 (카트 이미지 위에 표시)
	// ─────────────────────────────────────────────────

	// 차량 정보 패널 (텍스트들을 묶어서 회전)
	VehicleInfoPanel = MakeShared<SPanel>();
	VehicleInfoPanel->SetRotation(-10.f);  // 패널 전체를 기울임

	// 속도 텍스트
	VehicleSpeedText = MakeShared<STextBlock>();
	VehicleSpeedText->SetText(L"0 km/h")
		.SetFontSize(24.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

	// RPM 텍스트
	VehicleRpmText = MakeShared<STextBlock>();
	VehicleRpmText->SetText(L"0 RPM")
		.SetFontSize(16.f)
		.SetColor(FSlateColor(0.8f, 0.8f, 0.8f, 1.f))
		.SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black());

	// 기어 텍스트
	VehicleGearText = MakeShared<STextBlock>();
	VehicleGearText->SetText(L"GEAR: N")
		.SetFontSize(18.f)
		.SetColor(FSlateColor(0.4f, 0.9f, 0.35f, 1.f))
		.SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black());

	// 패널에 자식 추가 (패널 내부 상대 좌표)
	FSlot& SpeedSlot = VehicleInfoPanel->AddChild(VehicleSpeedText);
	SpeedSlot.Position = FVector2D(0.f, 0.f);
	SpeedSlot.Size = FVector2D(200.f, 40.f);

	FSlot& RpmSlot = VehicleInfoPanel->AddChild(VehicleRpmText);
	RpmSlot.Position = FVector2D(0.f, 30.f);  // 40 → 25 (간격 줄임)
	RpmSlot.Size = FVector2D(200.f, 30.f);

	FSlot& GearSlot = VehicleInfoPanel->AddChild(VehicleGearText);
	GearSlot.Position = FVector2D(0.f, 53.f);  // 70 → 50 (간격 줄임)
	GearSlot.Size = FVector2D(200.f, 30.f);

	// 패널을 HUD에 추가
	SGameHUD::Get().AddWidget(VehicleInfoPanel)
		.SetAnchor(0.0f, 1.0f)  // 좌하단
		.SetPivot(0.0f, 1.0f)   // 좌하단 기준
		.SetOffset(145.f, -122.f)  // 아래로 내림 (-180 → -160)
		.SetSize(200.f, 100.f);  // 3개 텍스트를 담을 크기

	// ─────────────────────────────────────────────────
	// 미니맵 (오른쪽 하단)
	// ─────────────────────────────────────────────────

	Minimap = MakeShared<SMinimap>();
	Minimap->SetMapTexture(L"Data/Textures/Dumb/Minimap.png")  // 미니맵 이미지
		.SetPlayerMarkerTexture(L"Data/Textures/Dumb/PlayerMarker.png")  // 플레이어 마커
		.SetRingTexture(L"Data/Textures/Dumb/MapRing.png")  // 미니맵 테두리 링
		.SetRingHighQuality(true)  // 링 텍스처 고품질 보간
		.SetWorldBounds(FVector(-278.5f, -116.5f, 0.f), FVector(278.5f, 116.5f, 0.f))  // 실제 맵 크기 (X: 557, Y: 233)
		.SetMarkerSize(18.f)  // 마커 크기
		.SetZoomLevel(150.f);  // 플레이어 주변 150 유닛 반경 표시 (작을수록 확대됨)

	SGameHUD::Get().AddWidget(Minimap)
		.SetAnchor(1.0f, 1.0f)  // 오른쪽 하단
		.SetPivot(1.0f, 1.0f)   // 오른쪽 하단 기준
		.SetOffset(-40.f, -60.f)  // 위로 올림 (-20 → -60)
		.SetSize(180.f, 180.f);   // 정사각형 미니맵

	// ─────────────────────────────────────────────────
	// 경과 시간 UI (상단 오른쪽)
	// ─────────────────────────────────────────────────

	// 배경 박스 (반투명 검정)
	ElapsedTimeBg = MakeShared<SBorderBox>();
	ElapsedTimeBg->SetBackgroundColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetBorderThickness(2.f)
		.SetBorderColor(FSlateColor(1.0f, 1.0f, 1.0f, 0.3f));  // 밝은 회색 테두리

	SGameHUD::Get().AddWidget(ElapsedTimeBg)
		.SetAnchor(1.0f, 0.0f)  // 우상단
		.SetPivot(1.0f, 0.0f)   // 우상단 기준
		.SetOffset(-20.f, 20.f)  // 우상단에서 조금 안쪽
		.SetSize(180.f, 50.f);

	// 경과 시간 텍스트
	ElapsedTimeText = MakeShared<STextBlock>();
	ElapsedTimeText->SetText(L"TIME: 00:00")
		.SetFontSize(24.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Center)
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(ElapsedTimeText)
		.SetAnchor(1.0f, 0.0f)  // 우상단
		.SetPivot(1.0f, 0.0f)   // 우상단 기준
		.SetOffset(-20.f, 20.f)  // 배경과 동일한 위치
		.SetSize(180.f, 50.f);

	// ─────────────────────────────────────────────────
	// 튜토리얼 만화/컷씬 (전체 화면, 7장)
	// ─────────────────────────────────────────────────

	for (int32 i = 0; i < 7; i++)
	{
		TSharedPtr<SImage> ComicImage = MakeShared<SImage>();

		// 파일명: Comic_01.png, Comic_02.png, ...
		wchar_t FileName[256];
		swprintf_s(FileName, L"Data/Textures/Dumb/Comic_%02d.png", i + 1);

		ComicImage->SetTexture(FileName);

		// 전체 화면을 채우도록 설정 (상대 크기 사용)
		SGameHUD::Get().AddWidget(ComicImage)
			.SetAnchor(0.0f, 0.0f)      // 좌상단 앵커
			.SetPivot(0.0f, 0.0f)       // 좌상단 피벗
			.SetOffset(0.f, 0.f)        // 오프셋 없음
			.SetRelativeSize(1.0f, 1.0f);  // 부모(뷰포트) 크기의 100% (전체 화면)

		// 초기에는 모두 숨김
		ComicImage->SetVisibility(ESlateVisibility::Hidden);

		ComicImages.Add(ComicImage);
	}

	// ─────────────────────────────────────────────────
	// 카메라 찾기 (이름으로 검색)
	// ─────────────────────────────────────────────────

	// 타이틀 화면 카메라
	AActor* FoundTitleCamera = GetWorld()->FindActorByName(FName("TitleCamera"));
	if (FoundTitleCamera)
	{
		TitleCamera = Cast<ACameraActor>(FoundTitleCamera);
	}

	// 튜토리얼 카메라
	AActor* FoundTutorialCamera = GetWorld()->FindActorByName(FName("TutorialCamera"));
	if (FoundTutorialCamera)
	{
		TutorialCamera = Cast<ACameraActor>(FoundTutorialCamera);
	}

	// ─────────────────────────────────────────────────
	// 초기 상태: 메인 메뉴 표시
	// ─────────────────────────────────────────────────

	ShowMainMenu();

	// ─────────────────────────────────────────────────
	// 플레이어 입력 비활성화 (타이틀 화면 동안)
	// 카메라는 기본 스프링암 카메라 사용
	// ─────────────────────────────────────────────────

	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		PC->SetInputEnabled(false);  // 플레이어 입력 비활성화
	}
}

void AHudExampleGameMode::EndPlay()
{
	Super::EndPlay();

	// HUD 위젯 정리
	if (SGameHUD::Get().IsInitialized())
	{
		if (TitleImage)
		{
			SGameHUD::Get().RemoveWidget(TitleImage);
			TitleImage.Reset();
		}
		if (StartButton)
		{
			SGameHUD::Get().RemoveWidget(StartButton);
			StartButton.Reset();
		}
		if (PressSpaceBg)
		{
			SGameHUD::Get().RemoveWidget(PressSpaceBg);
			PressSpaceBg.Reset();
		}
		if (PressSpaceImage)
		{
			SGameHUD::Get().RemoveWidget(PressSpaceImage);
			PressSpaceImage.Reset();
		}
		if (PressSpaceBg2)
		{
			SGameHUD::Get().RemoveWidget(PressSpaceBg2);
			PressSpaceBg2.Reset();
		}
		if (PressSpaceImage2)
		{
			SGameHUD::Get().RemoveWidget(PressSpaceImage2);
			PressSpaceImage2.Reset();
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
		if (VehicleInfoPanel)
		{
			SGameHUD::Get().RemoveWidget(VehicleInfoPanel);
			VehicleInfoPanel.Reset();
		}
		// VehicleSpeedText, VehicleRpmText, VehicleGearText는 VehicleInfoPanel의 자식이므로 별도 정리 불필요
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
		if (CartImage)
		{
			SGameHUD::Get().RemoveWidget(CartImage);
			CartImage.Reset();
		}
		// 만화 이미지들 정리
		for (auto& Comic : ComicImages)
		{
			if (Comic)
			{
				SGameHUD::Get().RemoveWidget(Comic);
				Comic.Reset();
			}
		}
		ComicImages.Empty();
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 상태 관리
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::ShowMainMenu()
{
	CurrentGameState = EGameState::MainMenu;
	MainMenuTimer = 0.f;  // 타이머 리셋
	bReadyForInput = false;  // 입력 대기 상태 리셋

	// 메인 메뉴 UI 표시
	if (TitleImage)
		TitleImage->SetVisibility(ESlateVisibility::Visible);

	// "Press Space" 이미지는 초기에 숨김 (애니메이션 완료 후 표시)
	if (PressSpaceBg)
		PressSpaceBg->SetVisibility(ESlateVisibility::Hidden);
	if (PressSpaceImage)
		PressSpaceImage->SetVisibility(ESlateVisibility::Hidden);

	// 시작 버튼은 숨김 (자동 전환)
	if (StartButton)
		StartButton->SetVisibility(ESlateVisibility::Hidden);

	// 게임 플레이 UI 숨기기
	if (ScoreText) ScoreText->SetVisibility(ESlateVisibility::Hidden);
	if (TestImage) TestImage->SetVisibility(ESlateVisibility::Hidden);
	if (BoxesIcon) BoxesIcon->SetVisibility(ESlateVisibility::Hidden);
	if (BoxesLeftText) BoxesLeftText->SetVisibility(ESlateVisibility::Hidden);
	if (BoxesCountText) BoxesCountText->SetVisibility(ESlateVisibility::Hidden);
	if (ReachHomeBg) ReachHomeBg->SetVisibility(ESlateVisibility::Hidden);
	if (ReachHomeText) ReachHomeText->SetVisibility(ESlateVisibility::Hidden);
	if (CartImage) CartImage->SetVisibility(ESlateVisibility::Hidden);
	if (VehicleInfoPanel) VehicleInfoPanel->SetVisibility(ESlateVisibility::Hidden);
	if (Minimap) Minimap->SetVisibility(ESlateVisibility::Hidden);
	if (ElapsedTimeBg) ElapsedTimeBg->SetVisibility(ESlateVisibility::Hidden);
	if (ElapsedTimeText) ElapsedTimeText->SetVisibility(ESlateVisibility::Hidden);
}

void AHudExampleGameMode::StartCameraCinematic()
{
	CurrentGameState = EGameState::Tutorial_Camera;

	// 타이틀 UI 숨기기
	if (TitleImage)
		TitleImage->SetVisibility(ESlateVisibility::Hidden);
	if (PressSpaceBg)
		PressSpaceBg->SetVisibility(ESlateVisibility::Hidden);
	if (PressSpaceImage)
		PressSpaceImage->SetVisibility(ESlateVisibility::Hidden);

	// 플레이어 입력은 여전히 비활성화 유지 (튜토리얼 보는 동안)
	// 나중에 튜토리얼 종료 시 활성화

	// TutorialCamera로 전환 (블렌드)
	if (TutorialCamera)
	{
		APlayerController* PC = GetPlayerController();
		if (PC)
		{
			APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager();
			if (CameraManager)
			{
				UCameraComponent* TutorialCameraComp = TutorialCamera->GetCameraComponent();
				if (TutorialCameraComp)
				{
					// 블렌드 시간을 주고 카메라 전환
					CameraManager->SetViewCameraWithBlend(TutorialCameraComp, CameraBlendTime);
				}
			}
		}
	}

	// 타이머 리셋
	TutorialCameraTimer = 0.f;
	bTutorialCameraReady = false;
}

void AHudExampleGameMode::ShowTutorialComic()
{
	CurrentGameState = EGameState::Tutorial_Comic;

	// Press Any Key UI 숨기기 (두 번째 위젯)
	if (PressSpaceBg2)
		PressSpaceBg2->SetVisibility(ESlateVisibility::Hidden);
	if (PressSpaceImage2)
		PressSpaceImage2->SetVisibility(ESlateVisibility::Hidden);

	// 만화 초기화
	CurrentComicIndex = 0;
	ComicSceneTimer = 0.f;
	bComicInputReady = false;  // 입력 준비 안됨 (무시 시간 후 초기화 필요)

	// 첫 번째 만화 장면 표시
	if (ComicImages.Num() > 0 && ComicImages[0])
		ComicImages[0]->SetVisibility(ESlateVisibility::Visible);
}

void AHudExampleGameMode::StartGamePlay()
{
	CurrentGameState = EGameState::Playing;

	// 메인 메뉴 UI 숨기기
	if (TitleImage)
		TitleImage->SetVisibility(ESlateVisibility::Hidden);
	if (StartButton)
		StartButton->SetVisibility(ESlateVisibility::Hidden);

	// 만화 이미지들 모두 숨기기
	for (auto& Comic : ComicImages)
	{
		if (Comic)
			Comic->SetVisibility(ESlateVisibility::Hidden);
	}

	// 게임 플레이 UI 표시
	if (ScoreText) ScoreText->SetVisibility(ESlateVisibility::Visible);
	if (TestImage) TestImage->SetVisibility(ESlateVisibility::Visible);
	if (BoxesIcon) BoxesIcon->SetVisibility(ESlateVisibility::Visible);
	if (BoxesLeftText) BoxesLeftText->SetVisibility(ESlateVisibility::Visible);
	if (BoxesCountText) BoxesCountText->SetVisibility(ESlateVisibility::Visible);
	if (ReachHomeBg) ReachHomeBg->SetVisibility(ESlateVisibility::Visible);
	if (ReachHomeText) ReachHomeText->SetVisibility(ESlateVisibility::Visible);
	if (CartImage) CartImage->SetVisibility(ESlateVisibility::Visible);
	if (VehicleInfoPanel) VehicleInfoPanel->SetVisibility(ESlateVisibility::Visible);
	if (Minimap) Minimap->SetVisibility(ESlateVisibility::Visible);
	if (ElapsedTimeBg) ElapsedTimeBg->SetVisibility(ESlateVisibility::Visible);
	if (ElapsedTimeText) ElapsedTimeText->SetVisibility(ESlateVisibility::Visible);

	// 경과 시간 초기화
	ElapsedGameTime = 0.f;

	// ─────────────────────────────────────────────────
	// 실제 게임 시작: 플레이어를 시작 지점으로 이동
	// ─────────────────────────────────────────────────

	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		APawn* PlayerPawn = PC->GetPawn();
		if (PlayerPawn)
		{
			// 1. 시작 지점으로 텔레포트 (166.79, 1.3, 3)
			FVector GameStartLocation(166.79f, 1.3f, 3.0f);
			FQuat GameStartRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, 180.0f));
			PlayerPawn->SetActorLocation(GameStartLocation);
			PlayerPawn->SetActorRotation(GameStartRotation);

			// 2. 플레이어 카메라(SpringArm)로 복귀
			UCameraComponent* PawnCamera = Cast<UCameraComponent>(
				PlayerPawn->GetComponent(UCameraComponent::StaticClass())
			);
			if (PawnCamera)
			{
				APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager();
				if (CameraManager)
				{
					CameraManager->SetViewCamera(PawnCamera);
				}
			}

			// 3. 입력 활성화
			PC->SetInputEnabled(true);
		}
	}

	// 게임 시작
	StartGame();
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 라이프사이클
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::StartGame()
{
	Super::StartGame();
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

	// ─────────────────────────────────────────────────
	// 첫 프레임: 플레이어 카메라(스프링암)로 명시적 설정
	// ─────────────────────────────────────────────────

	if (!bInitialCameraSet && CurrentGameState == EGameState::MainMenu)
	{
		APlayerController* PC = GetPlayerController();
		if (PC)
		{
			APawn* PlayerPawn = PC->GetPawn();
			if (PlayerPawn)
			{
				// Pawn에서 카메라 컴포넌트 찾기
				UCameraComponent* PawnCamera = Cast<UCameraComponent>(
					PlayerPawn->GetComponent(UCameraComponent::StaticClass())
				);

				if (PawnCamera)
				{
					APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager();
					if (CameraManager)
					{
						// Pawn의 카메라를 명시적으로 설정 (TutorialCamera 무시)
						CameraManager->SetViewCamera(PawnCamera);
						bInitialCameraSet = true;  // 한 번만 실행
					}
				}
			}
		}
	}

	// ─────────────────────────────────────────────────
	// UI 애니메이션 업데이트
	// ─────────────────────────────────────────────────

	SGameHUD::Get().UpdateAnimations(DeltaSeconds);

	// ─────────────────────────────────────────────────
	// 메인 메뉴 입력 대기 (Space 또는 Enter만)
	// ─────────────────────────────────────────────────

	if (CurrentGameState == EGameState::MainMenu)
	{
		MainMenuTimer += DeltaSeconds;

		// 최소 대기 시간이 지나면 입력 대기 준비
		if (!bReadyForInput && MainMenuTimer >= MinWaitTime)
		{
			bReadyForInput = true;

			// "Press Space to Continue" 이미지 표시
			if (PressSpaceBg)
				PressSpaceBg->SetVisibility(ESlateVisibility::Visible);
			if (PressSpaceImage)
				PressSpaceImage->SetVisibility(ESlateVisibility::Visible);
		}

		// 깜박이는 효과 (느리고 부드럽게)
		if (bReadyForInput && PressSpaceImage)
		{
			// sin 함수로 부드러운 깜박임 (주기를 3초로 늘림)
			float BlinkSpeed = 2.5f;  // 2.0 → 3.0 (더 느리게)
			float Alpha = (std::sin(MainMenuTimer * 3.14159f * 2.0f / BlinkSpeed) + 1.0f) * 0.5f;  // 0~1 사이 값

			// 최소 투명도 0.5, 최대 0.8 사이로 조정 (너무 밝지 않게)
			float FinalAlpha = 0.5f + (Alpha * 0.4f);  // 0.5~0.9 사이

			// 이미지의 투명도 변경
			PressSpaceImage->SetOpacity(FinalAlpha);
		}

		// 입력 대기 준비 완료 후 아무 키나 체크
		if (bReadyForInput)
		{
			// 아무 키나 눌렸는지 체크 (0~255까지 모든 가상 키코드)
			bool bAnyKeyPressed = false;
			for (int key = 0; key < 256; key++)
			{
				if (GetAsyncKeyState(key) & 0x8000)
				{
					bAnyKeyPressed = true;
					break;
				}
			}

			if (bAnyKeyPressed)
			{
				// 튜토리얼 카메라로 전환
				StartCameraCinematic();
				return;
			}
		}
	}

	// ─────────────────────────────────────────────────
	// 튜토리얼 카메라 입력 대기
	// ─────────────────────────────────────────────────

	if (CurrentGameState == EGameState::Tutorial_Camera)
	{
		TutorialCameraTimer += DeltaSeconds;

		// 일정 시간 대기 후 입력 대기 준비
		if (!bTutorialCameraReady && TutorialCameraTimer >= TutorialCameraWaitTime)
		{
			bTutorialCameraReady = true;

			// "Press Any Key" 두 번째 위젯 표시 (위치 조정된 버전)
			if (PressSpaceBg2)
				PressSpaceBg2->SetVisibility(ESlateVisibility::Visible);
			if (PressSpaceImage2)
				PressSpaceImage2->SetVisibility(ESlateVisibility::Visible);
		}

		// 깜박이는 효과 (튜토리얼 카메라에서는 두 번째 위젯 사용)
		if (bTutorialCameraReady && PressSpaceImage2)
		{
			// sin 함수로 부드러운 깜박임
			float BlinkSpeed = 2.5f;
			float Alpha = (std::sin(TutorialCameraTimer * 3.14159f * 2.0f / BlinkSpeed) + 1.0f) * 0.5f;

			// 최소 투명도 0.5, 최대 0.9 사이로 조정
			float FinalAlpha = 0.5f + (Alpha * 0.4f);

			// 이미지의 투명도 변경
			PressSpaceImage2->SetOpacity(FinalAlpha);
		}

		// 입력 대기 준비 완료 후 아무 키나 체크
		if (bTutorialCameraReady)
		{
			bool bAnyKeyPressed = false;
			for (int key = 0; key < 256; key++)
			{
				if (GetAsyncKeyState(key) & 0x8000)
				{
					bAnyKeyPressed = true;
					break;
				}
			}

			if (bAnyKeyPressed)
			{
				// 만화/컷씬으로 전환
				ShowTutorialComic();
				return;
			}
		}
	}

	// ─────────────────────────────────────────────────
	// 만화/컷씬 진행
	// ─────────────────────────────────────────────────

	if (CurrentGameState == EGameState::Tutorial_Comic)
	{
		ComicSceneTimer += DeltaSeconds;

		// 키 입력 체크 (Space/Enter: 다음 장면, ESC: 전체 스킵)
		bool bNextScene = false;
		bool bSkipAll = false;

		// 입력 무시 시간이 지났을 때만 키 체크
		if (ComicSceneTimer >= ComicInputIgnoreTime)
		{
			// 첫 번째 프레임에서 키 상태 초기화 (이전 입력 무시)
			if (!bComicInputReady)
			{
				bComicInputReady = true;
				bPrevSpacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
				bPrevEnterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
				bPrevEscPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
			}
			else
			{
				// 현재 키 상태
				bool bSpacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
				bool bEnterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
				bool bEscPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

				// ESC 키 체크 (전체 스킵) - 눌렀다 뗐을 때 (Falling Edge)
				if (!bEscPressed && bPrevEscPressed)
				{
					bSkipAll = true;
				}
				// Space 또는 Enter 키 체크 (다음 장면) - 눌렀다 뗐을 때 (Falling Edge)
				else if ((!bSpacePressed && bPrevSpacePressed) ||
					(!bEnterPressed && bPrevEnterPressed))
				{
					bNextScene = true;
				}

				// 이전 키 상태 저장
				bPrevSpacePressed = bSpacePressed;
				bPrevEnterPressed = bEnterPressed;
				bPrevEscPressed = bEscPressed;
			}
		}

		// 자동 진행 (4초 경과)
		if (!bNextScene && !bSkipAll && ComicSceneTimer >= ComicSceneWaitTime)
		{
			bNextScene = true;
		}

		// 전체 스킵
		if (bSkipAll)
		{
			// 모든 만화 장면 숨김
			for (auto& Comic : ComicImages)
			{
				if (Comic)
					Comic->SetVisibility(ESlateVisibility::Hidden);
			}

			// 게임 플레이로 전환
			StartGamePlay();
			return;
		}

		// 다음 장면으로 이동
		if (bNextScene)
		{
			// 현재 장면 숨기기
			if (CurrentComicIndex < ComicImages.Num() && ComicImages[CurrentComicIndex])
				ComicImages[CurrentComicIndex]->SetVisibility(ESlateVisibility::Hidden);

			// 다음 장면 인덱스
			CurrentComicIndex++;

			// 마지막 장면이었으면 게임 플레이로
			if (CurrentComicIndex >= TotalComicScenes)
			{
				StartGamePlay();
				return;
			}

			// 다음 장면 표시
			if (CurrentComicIndex < ComicImages.Num() && ComicImages[CurrentComicIndex])
				ComicImages[CurrentComicIndex]->SetVisibility(ESlateVisibility::Visible);

			// 타이머 리셋
			ComicSceneTimer = 0.f;
		}
	}

	// ─────────────────────────────────────────────────
	// 게임 플레이 중일 때만 업데이트
	// ─────────────────────────────────────────────────

	if (CurrentGameState != EGameState::Playing)
		return;

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

	// ─────────────────────────────────────────────────
	// 경과 시간 업데이트
	// ─────────────────────────────────────────────────

	ElapsedGameTime += DeltaSeconds;

	if (ElapsedTimeText)
	{
		// 분:초 형식으로 표시 (MM:SS)
		int32 TotalSeconds = static_cast<int32>(ElapsedGameTime);
		int32 Minutes = TotalSeconds / 60;
		int32 Seconds = TotalSeconds % 60;

		wchar_t TimeStr[32];
		swprintf_s(TimeStr, L"TIME: %02d:%02d", Minutes, Seconds);
		ElapsedTimeText->SetText(TimeStr);
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
