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
#include "GameUI/SProgressBar.h"
#include "PlayerController.h"
#include "CargoComponent.h"
#include "CameraActor.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include <chrono>
#include <ctime>

// ────────────────────────────────────────────────────────────────────────────
// 정적 변수 정의
// ────────────────────────────────────────────────────────────────────────────

bool AHudExampleGameMode::bSkipTutorialOnRestart = false;

// ────────────────────────────────────────────────────────────────────────────
// 생성자
// ────────────────────────────────────────────────────────────────────────────

AHudExampleGameMode::AHudExampleGameMode()
{
	// DefaultPawnClass = ADancingCharacter::StaticClass();
	DefaultPawnClass = AVehicle::StaticClass();

	MainMenuMusicComponent = CreateDefaultSubobject<UAudioComponent>("");
	USound* MainMenuMusic = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/confessin.wav");
	MainMenuMusicComponent->SetSound(MainMenuMusic);
	MainMenuMusicComponent->bAutoPlay = false;
	MainMenuMusicComponent->bIsLooping = true;
	MainMenuMusicComponent->bIsUISound = true;
	MainMenuMusicComponent->Volume = 0.2f;
	
	BackgroundMusicComponent = CreateDefaultSubobject<UAudioComponent>("");
	USound* BackgroundMusic = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/confessin.wav");
	BackgroundMusicComponent->SetSound(BackgroundMusic);
	BackgroundMusicComponent->bAutoPlay = false;
	BackgroundMusicComponent->bIsLooping = true;
	BackgroundMusicComponent->bIsUISound = true;
	BackgroundMusicComponent->Volume = 0.2f;
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!SGameHUD::Get().IsInitialized())
		return;

	SGameHUD::Get().ReserveRootCanvasSlots(50);
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
	// 게임 플레이 UI - 점수판 (좌상단)
	// ─────────────────────────────────────────────────

	// 박스 아이콘 (좌상단으로 이동)
	BoxesIcon = MakeShared<SImage>();
	BoxesIcon->SetTexture(L"Data/Textures/Dumb/newbox.png")
		.SetHighQualityInterpolation(true);
		
	SGameHUD::Get().AddWidget(BoxesIcon)
		.SetAnchor(0.0f, 0.0f)  // 좌상단
		.SetPivot(0.0f, 0.0f)   // 좌상단 기준
		.SetOffset(20.f, 20.f)
		.SetSize(150.f, 150.f);

	// "x BOXES LEFT" 배경 그라데이션 (박스 아이콘 오른쪽 아래)
	BoxesLeftBg = MakeShared<SGradientBox>();
	BoxesLeftBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetFadeWidth(60.f);

	SGameHUD::Get().AddWidget(BoxesLeftBg)
		.SetAnchor(0.0f, 0.0f)  // 좌상단
		.SetPivot(0.0f, 0.0f)   // 좌상단 기준
		.SetOffset(105.f, 132.f)  // 박스 아이콘 (20+150=170) 오른쪽 (+10), 아래쪽 (20+150-40-5=135)
		.SetSize(90.f, 20.f);

	// "x BOXES LEFT" 텍스트
	BoxesLeftText = MakeShared<SImage>();
	BoxesLeftText->SetTexture(L"Data/Textures/Dumb/boxeslefttext.png")
		.SetHighQualityInterpolation(true);

	SGameHUD::Get().AddWidget(BoxesLeftText)
		.SetAnchor(0.0f, 0.0f)  // 좌상단
		.SetPivot(0.0f, 0.0f)   // 좌상단 기준
		.SetOffset(120.f, 133.f)  // 배경 중앙 정렬: 105 + (90-60)/2 = 120
		.SetSize(60.f, 20.f);

	// 박스 개수 텍스트 (박스 아이콘 중앙)
	BoxesCountText = MakeShared<STextBlock>();
	BoxesCountText->SetText(L"x 0")
		.SetFontSize(27.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Right)  // 오른쪽 정렬: 숫자가 바뀌어도 오른쪽 끝 고정
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(BoxesCountText)
		.SetAnchor(0.0f, 0.0f)  // 좌상단
		.SetPivot(0.0f, 0.0f)   // 좌상단 기준
		.SetOffset(130.f, 88.f)  // 박스 아이콘 중앙에 위치하도록 조정
		.SetSize(50.f, 50.f);

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
	// "TO HOME" 배경 그라데이션 (진행 바 왼쪽)
	// ─────────────────────────────────────────────────

	ToHomeBg = MakeShared<SGradientBox>();
	ToHomeBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetFadeWidth(60.f);  // boost와 동일

	SGameHUD::Get().AddWidget(ToHomeBg)
		.SetAnchor(1.0f, 1.0f)  // 오른쪽 하단
		.SetPivot(1.0f, 1.0f)   // 오른쪽 하단 기준
		.SetOffset(-230.f, -37.f)  // 진행 바 중앙에 정렬
		.SetSize(110.f, 18.f);  // boost와 동일 (110x18)

	// "TO HOME" 텍스트 (배경 중앙에 배치)
	ToHomeText = MakeShared<SImage>();
	ToHomeText->SetTexture(L"Data/Textures/Dumb/tohome.png");

	SGameHUD::Get().AddWidget(ToHomeText)
		.SetAnchor(1.0f, 1.0f)  // 오른쪽 하단
		.SetPivot(0.5f, 0.5f)   // 중앙 기준 (중앙 정렬)
		.SetOffset(-285.f, -46.f)  // 그라데이션 박스 중앙 (-230 - 110/2 = -285)
		.SetSize(60.f, 12.f);  // boost와 동일

	// ─────────────────────────────────────────────────
	// 목적지까지 거리 진행 바 (미니맵 하단에 위치, 먼저 그려서 뒤에 렌더링)
	// ─────────────────────────────────────────────────

	DistanceProgressBar = MakeShared<SProgressBar>();
	DistanceProgressBar->SetPercent(0.7f)  // 예제: 70% 진행
		.SetBarColor(FSlateColor(146.f/255.f, 254.f/255.f, 131.f/255.f, 1.0f))  // rgb(146, 254, 131)
		.SetBackgroundColor(FSlateColor(0.2f, 0.2f, 0.2f, 0.4f))  // 어두운 배경 (더 투명하게: 0.8 → 0.4)
		.SetBorderColor(FSlateColor(0.5f, 0.5f, 0.5f, 1.0f))  // 회색 테두리
		.SetBorderThickness(2.0f)
		.SetCornerRadius(4.0f);

	SGameHUD::Get().AddWidget(DistanceProgressBar)
		.SetAnchor(1.0f, 1.0f)  // 오른쪽 하단 (미니맵과 동일)
		.SetPivot(1.0f, 1.0f)   // 오른쪽 하단 기준
		.SetOffset(-40.f, -40.f)  // 미니맵 바로 아래 (미니맵: -60, 미니맵 크기 180 → -60 + 180 = 120, 여기서 위로 조금: -40)
		.SetSize(180.f, 12.f);  // 미니맵과 같은 너비, 세로 12px (얇게)

	// ─────────────────────────────────────────────────
	// 부스터 게이지 UI (좌하단, 차량 정보 아래)
	// ─────────────────────────────────────────────────

	// "BOOSTER" 배경 그라데이션 (진행 바 왼쪽)
	BoosterTextBg = MakeShared<SGradientBox>();
	BoosterTextBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f))  // 반투명 검정
		.SetFadeWidth(60.f);  // 80 → 60

	SGameHUD::Get().AddWidget(BoosterTextBg)
		.SetAnchor(0.0f, 1.0f)  // 왼쪽 하단
		.SetPivot(0.0f, 1.0f)   // 왼쪽 하단 기준
		.SetOffset(20.f, -37.f)  // 진행 바 중앙에 정렬
		.SetSize(100.f, 18.f);  // 120 → 100

	// "BOOSTER" 텍스트 이미지
	BoosterText = MakeShared<SImage>();
	BoosterText->SetTexture(L"Data/Textures/Dumb/boost.png");

	SGameHUD::Get().AddWidget(BoosterText)
		.SetAnchor(0.0f, 1.0f)  // 왼쪽 하단
		.SetPivot(0.5f, 0.5f)   // 중앙 기준 (중앙 정렬)
		.SetOffset(70.f, -46.f)  // 그라데이션 박스 중앙 (20 + 100/2 = 70)
		.SetSize(60.f, 12.f);

	// 부스터 게이지 진행 바
	BoosterProgressBar = MakeShared<SProgressBar>();
	BoosterProgressBar->SetPercent(0.0f)  // 초기값: 0%
		.SetBarColor(FSlateColor(255.f/255.f, 200.f/255.f, 100.f/255.f, 1.0f))  // rgb(255, 200, 100) - 불꽃색
		.SetBackgroundColor(FSlateColor(0.2f, 0.2f, 0.2f, 0.4f))  // 어두운 배경
		.SetBorderColor(FSlateColor(0.5f, 0.5f, 0.5f, 1.0f))  // 회색 테두리
		.SetBorderThickness(2.0f)
		.SetCornerRadius(4.0f);

	SGameHUD::Get().AddWidget(BoosterProgressBar)
		.SetAnchor(0.0f, 1.0f)  // 왼쪽 하단
		.SetPivot(0.0f, 1.0f)   // 왼쪽 하단 기준
		.SetOffset(130.f, -40.f)  // "BOOSTER" 텍스트 오른쪽 (배경 줄어든만큼 왼쪽으로)
		.SetSize(162.f, 12.f);  // 180 → 162 (10% 감소)

	// ─────────────────────────────────────────────────
	// 미니맵 (오른쪽 하단, 진행 바 위에 렌더링)
	// ─────────────────────────────────────────────────

	Minimap = MakeShared<SMinimap>();
	Minimap->SetMapTexture(L"Data/Textures/Dumb/newmap.png")  // 미니맵 이미지
		.SetPlayerMarkerTexture(L"Data/Textures/Dumb/PlayerMarker.png")  // 플레이어 마커
		.SetRingTexture(L"Data/Textures/Dumb/MapRing.png")  // 미니맵 테두리 링
		.SetRingHighQuality(true)  // 링 텍스처 고품질 보간
		.SetWorldBounds(FVector(-9.5f, -386.0f, 0.f), FVector(224.0f, 172.0f, 0.f))  // 실제 맵 크기 (newmap.png: 1497x627)
		.SetMarkerSize(18.f)  // 마커 크기
		.SetZoomLevel(150.f);  // 플레이어 주변 150 유닛 반경 표시 (작을수록 확대됨)

	SGameHUD::Get().AddWidget(Minimap)
		.SetAnchor(1.0f, 1.0f)  // 오른쪽 하단
		.SetPivot(1.0f, 1.0f)   // 오른쪽 하단 기준
		.SetOffset(-40.f, -65.f)  // 위로 올림 (-20 → -60)
		.SetSize(180.f, 180.f);   // 정사각형 미니맵

	// ─────────────────────────────────────────────────
	// 날짜 UI (상단 오른쪽)
	// ─────────────────────────────────────────────────

	// 날짜 배경 (time.png - 둥근 네모 테두리)
	TestImage = MakeShared<SImage>();
	TestImage->SetTexture(L"Data/Textures/Dumb/time.png")
		.SetHighQualityInterpolation(true);  // 고품질 보간 적용

	SGameHUD::Get().AddWidget(TestImage)
		.SetAnchor(1.0f, 0.0f)  // 우상단
		.SetPivot(1.0f, 0.0f)   // 우상단 기준
		.SetOffset(-35.f, 25.f)
		.SetSize(180.f, 100.f);  // 크기 증가

	// 날짜 텍스트 (작은 글씨)                                                                                     
	//ElapsedTimeText = MakeShared<STextBlock>();                                                                  
	//ElapsedTimeText->SetText(L"") // Tick에서 업데이트                                                           
	//    .SetFontSize(14.f) // 글자 크기 감소                                                                     
	//    .SetColor(FSlateColor::White())                                                                          
	//    .SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black())                                              
	//    .SetHAlign(ETextHAlign::Center)                                                                          
	//    .SetVAlign(ETextVAlign::Center);

	//SGameHUD::Get().AddWidget(ElapsedTimeText)                                                                   
	//    .SetAnchor(1.0f, 0.0f)                                                                                   
	//    .SetPivot(1.0f, 0.0f)                                                                                    
	//    .SetOffset(-20.f, 30.f) // 상단에 배치                                                                   
	//    .SetSize(300.f, 30.f);
	SGameHUD::Get().AddWidget(ElapsedTimeText)
		.SetAnchor(0.5f, 0.0f)
		.SetPivot(0.5f, 0.0f)
		.SetOffset(0.f, 150.f) // 디버그 정보가 잘 보이도록 위치 조정
		.SetSize(400.f, 40.f);

	// "ELAPSED TIME" 레이블
	ScoreText = MakeShared<STextBlock>();
	ScoreText->SetText(L"ELAPSED TIME")
		.SetFontSize(16.f)
		.SetColor(FSlateColor(0.8f, 0.8f, 0.8f, 1.0f))
		.SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Center)
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(ScoreText)
		.SetAnchor(1.0f, 0.0f)
		.SetPivot(1.0f, 0.0f)
		.SetOffset(-35.f, 38.f)
		.SetSize(180.f, 30.f);

	// 경과 시간 텍스트 (중간 크기 폰트)
	ScoreValueText = MakeShared<STextBlock>();
	ScoreValueText->SetText(L"00:00")
		.SetFontSize(34.f) // 폰트 크기 조정
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Center)
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(ScoreValueText)
		.SetAnchor(1.0f, 0.0f)
		.SetPivot(1.0f, 0.0f) // 피벗 수정
		.SetOffset(-35.f, 65.f) // 레이블 아래에 배치
		.SetSize(180.f, 43.f);

	// ─────────────────────────────────────────────────
	// Objective 연출 (게임 시작 시 중앙에서 등장)
	// ─────────────────────────────────────────────────

	// Objective 배경 그라데이션
	ObjectiveBg = MakeShared<SGradientBox>();
	ObjectiveBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.7f))  // 반투명 검정
		.SetFadeWidth(200.f);

	ObjectiveBgSlot = &SGameHUD::Get().AddWidget(ObjectiveBg)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, 0.f)
		.SetSize(600.f, 60.f);

	// Objective 이미지
	ObjectiveImage = MakeShared<SImage>();
	ObjectiveImage->SetTexture(L"Data/Textures/Dumb/objective.png")
		.SetHighQualityInterpolation(true);

	ObjectiveImageSlot = &SGameHUD::Get().AddWidget(ObjectiveImage)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, 0.f)
		.SetSize(500.f, 40.f);

	// 초기에는 숨김 (게임 시작 시 표시)
	ObjectiveBg->SetVisibility(ESlateVisibility::Hidden);
	ObjectiveImage->SetVisibility(ESlateVisibility::Hidden);

	// ─────────────────────────────────────────────────
	// 튜토리얼 만화/컷씬 (전체 화면, 7장)
	// ─────────────────────────────────────────────────

	//if (GEngine.IsRestartPIE())
	//{
	//	CurrentGameState = EGameState::Playing;
	//	return;
	//}

	// 만화 UI는 맨 마지막에 추가 (다른 모든 UI 위에 그려지도록)

	// ─────────────────────────────────────────────────
	// 엔딩 크레딧 UI
	// ─────────────────────────────────────────────────

	// 엔딩 크레딧 배경 (검정색 전체 화면)
	CreditBackground = MakeShared<SImage>();
	CreditBackground->SetTint(FSlateColor::Black());

	SGameHUD::Get().AddWidget(CreditBackground)
		.SetAnchor(0.0f, 0.0f)
		.SetPivot(0.0f, 0.0f)
		.SetOffset(0.f, 0.f)
		.SetRelativeSize(1.0f, 1.0f);  // 전체 화면

	CreditBackground->SetVisibility(ESlateVisibility::Hidden);

	// 엔딩 크레딧 이미지 (세로로 긴 이미지, 스크롤)
	CreditImage = MakeShared<SImage>();
	CreditImage->SetTexture(L"Data/Textures/Dumb/endingcredit.png")
		.SetHighQualityInterpolation(true);

	CreditImageSlot = &SGameHUD::Get().AddWidget(CreditImage)
		.SetAnchor(0.0f, 0.0f)  // 좌상단 (0, 0)
		.SetPivot(0.0f, 0.0f)   // 좌상단 기준
		.SetOffset(0.f, 0.f)
		.SetSize(100.f, 100.f);  // 임시 크기 (ShowEndingCredits에서 재계산)

	CreditImage->SetVisibility(ESlateVisibility::Hidden);

	// ─────────────────────────────────────────────────
	// 조작키 설명서 UI (Tab 키로 토글)
	// ─────────────────────────────────────────────────

	// 반투명 배경
	KeyBindingsBackground = MakeShared<SBorderBox>();
	KeyBindingsBackground->SetBorderColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.6f));  // 검정색 80% 불투명

	SGameHUD::Get().AddWidget(KeyBindingsBackground)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, 0.f)
		.SetSize(1920.f, 1080.f);  // 전체 화면

	KeyBindingsBackground->SetVisibility(ESlateVisibility::Hidden);

	// 조작키 설명서 이미지
	KeyBindingsImage = MakeShared<SImage>();
	KeyBindingsImage->SetTexture(L"Data/Textures/Dumb/KeyBindings.png")
		.SetHighQualityInterpolation(true);

	SGameHUD::Get().AddWidget(KeyBindingsImage)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, 0.f)
		.SetSize(800.f, 600.f);  // 조작키 설명서 크기 (필요시 조정)

	KeyBindingsImage->SetVisibility(ESlateVisibility::Hidden);

	// ─────────────────────────────────────────────────
	// 게임 오버 / 일시정지 메뉴 UI (맨 마지막에 생성하여 최상위 렌더링)
	// ─────────────────────────────────────────────────

	// 게임 오버 배경 그라데이션
	GameOverBg = MakeShared<SGradientBox>();
	GameOverBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.8f))  // 반투명 검정
		.SetFadeWidth(150.f);

	SGameHUD::Get().AddWidget(GameOverBg)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, -240.f)
		.SetSize(600.f, 300.f);

	// "MISSION COMPLETE!" 텍스트
	GameOverText = MakeShared<STextBlock>();
	GameOverText->SetText(L"MISSION COMPLETE!")
		.SetFontSize(48.f)
		.SetColor(FSlateColor(146.f / 255.f, 254.f / 255.f, 131.f / 255.f, 1.0f))  // 연두색
		.SetShadow(true, FVector2D(3.f, 3.f), FSlateColor::Black())
		.SetHAlign(ETextHAlign::Center)
		.SetVAlign(ETextVAlign::Center);

	SGameHUD::Get().AddWidget(GameOverText)
		.SetAnchor(0.5f, 0.5f)  // 화면 중앙
		.SetPivot(0.5f, 0.5f)   // 중앙 기준
		.SetOffset(0.f, -240.f)
		.SetSize(500.f, 60.f);

	// 재시작 버튼
	RestartButton = MakeShared<SButton>();
	RestartButton->SetText(L"Restart Game")
		.SetFontSize(24.f)
		.SetTextColor(FSlateColor::White())
		.SetBackgroundColors(
			FSlateColor(0.2f, 0.2f, 0.2f, 0.8f),   // Normal
			FSlateColor(0.4f, 0.4f, 0.4f, 0.9f),   // Hovered
			FSlateColor(0.1f, 0.1f, 0.1f, 1.0f))   // Pressed
		.OnClicked([this]() {
			RestartGame();
		});

	SGameHUD::Get().AddWidget(RestartButton)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, -30.f)
		.SetSize(300.f, 50.f);

	// 엔딩 크레딧 버튼 (일시정지 메뉴에서는 "Resume" 버튼으로 사용)
	CreditsButton = MakeShared<SButton>();
	CreditsButton->SetText(L"Ending Credits")
		.SetFontSize(24.f)
		.SetTextColor(FSlateColor::White())
		.SetBackgroundColors(
			FSlateColor(0.2f, 0.2f, 0.2f, 0.8f),   // Normal
			FSlateColor(0.4f, 0.4f, 0.4f, 0.9f),   // Hovered
			FSlateColor(0.1f, 0.1f, 0.1f, 1.0f))   // Pressed
		.OnClicked([this]() {
			// 일시정지 상태라면 게임 재개
			if (CurrentGameState == EHudGameState::Paused)
			{
				ResumeGame();
			}
			// 엔딩 메뉴 상태라면 크레딧 표시
			else if (CurrentGameState == EHudGameState::EndMenu)
			{
				ShowEndingCredits();
			}
		});

	SGameHUD::Get().AddWidget(CreditsButton)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, 30.f)
		.SetSize(300.f, 50.f);

	// 게임 종료 버튼
	QuitButton = MakeShared<SButton>();
	QuitButton->SetText(L"Quit Game")
		.SetFontSize(24.f)
		.SetTextColor(FSlateColor::White())
		.SetBackgroundColors(
			FSlateColor(0.2f, 0.2f, 0.2f, 0.8f),   // Normal
			FSlateColor(0.4f, 0.4f, 0.4f, 0.9f),   // Hovered
			FSlateColor(0.1f, 0.1f, 0.1f, 1.0f))   // Pressed
		.OnClicked([this]() {
			PostQuitMessage(0);
		});

	SGameHUD::Get().AddWidget(QuitButton)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, 90.f)
		.SetSize(300.f, 50.f);

	// ─────────────────────────────────────────────────
	// WASTED 연출
	// ─────────────────────────────────────────────────

	// 배경 박스 (가로로 긴 그라데이션)
	WastedBackground = MakeShared<SGradientBox>();
	WastedBackground->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.0f));  // 투명 (초기)

	SGameHUD::Get().AddWidget(WastedBackground)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, 0.f)
		.SetSize(1920.f, 200.f);  // 화면 가로 전체, 높이 200

	// WASTED 이미지
	WastedImage = MakeShared<SImage>();
	WastedImage->SetTexture(L"Data/Textures/Dumb/wasted.png")
		.SetOpacity(0.0f)
		.SetHighQualityInterpolation(true);

	SGameHUD::Get().AddWidget(WastedImage)
		.SetAnchor(0.5f, 0.5f)
		.SetPivot(0.5f, 0.5f)
		.SetOffset(0.f, 0.f)
		.SetSize(600.f, 100.f);

	// 초기에는 숨김
	GameOverBg->SetVisibility(ESlateVisibility::Hidden);
	GameOverText->SetVisibility(ESlateVisibility::Hidden);
	RestartButton->SetVisibility(ESlateVisibility::Hidden);
	CreditsButton->SetVisibility(ESlateVisibility::Hidden);
	QuitButton->SetVisibility(ESlateVisibility::Hidden);
	WastedBackground->SetVisibility(ESlateVisibility::Hidden);
	WastedImage->SetVisibility(ESlateVisibility::Hidden);

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
	// 초기 상태: 재시작 플래그 확인
	// ─────────────────────────────────────────────────
	bool bSkipTutorialOnRestartCopy = bSkipTutorialOnRestart;  // 플래그 복사
	if (bSkipTutorialOnRestart)
	{
		// 튜토리얼 스킵하고 바로 게임 시작
		bSkipTutorialOnRestart = false;  // 플래그 리셋
		StartGamePlay();
	}
	else
	{
		// 정상적인 메인 메뉴 표시
		ShowMainMenu();

		// 플레이어 입력 비활성화 (타이틀 화면 동안)
		APlayerController* PC = GetPlayerController();
		if (PC)
		{
			PC->SetInputEnabled(false);  // 플레이어 입력 비활성화
		}
	}
	
	// ─────────────────────────────────────────────────
	// 만화 UI (맨 마지막에 추가하여 최상위 레이어로)
	// ─────────────────────────────────────────────────

	// 만화 배경 (반투명 검정색 전체 화면)
	ComicBackground = MakeShared<SBorderBox>();
	ComicBackground->SetBackgroundColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.7f))  // 70% 불투명도
		.SetBorderThickness(0.f);  // 테두리 없음
	SGameHUD::Get().AddWidget(ComicBackground)
		.SetAnchor(0.0f, 0.0f)
		.SetPivot(0.0f, 0.0f)
		.SetOffset(0.f, 0.f)
		.SetRelativeSize(1.0f, 1.0f);  // 전체 화면
	ComicBackground->SetVisibility(ESlateVisibility::Hidden);

	// 만화 이미지들 (8장)
	for (int32 i = 0; i < 8; i++)
	{
		TSharedPtr<SImage> ComicImage = MakeShared<SImage>();

		// 파일명: Comic_01.png, Comic_02.png, ...
		wchar_t FileName[256];
		swprintf_s(FileName, L"Data/Textures/Dumb/Comic_%02d.png", i + 1);

		ComicImage->SetTexture(FileName);

		// 화면 전체 크기로 설정
		FCanvasSlot* Slot = &SGameHUD::Get().AddWidget(ComicImage)
			.SetAnchor(0.0f, 0.0f)      // 좌상단 앵커
			.SetPivot(0.0f, 0.0f)       // 좌상단 피벗
			.SetOffset(0.f, 0.f)        // 초기 위치 (ShowTutorialComic에서 설정)
			.SetRelativeSize(1.0f, 1.0f);  // 뷰포트 크기

		// 초기에는 모두 숨김
		ComicImage->SetVisibility(ESlateVisibility::Hidden);

		ComicImages.Add(ComicImage);
		ComicImageSlots.Add(Slot);
	}

	// ─────────────────────────────────────────────────
	// 플레이어 입력 비활성화 (타이틀 화면 동안)
	// 카메라는 기본 스프링암 카메라 사용
	// ─────────────────────────────────────────────────

	if (!bSkipTutorialOnRestartCopy && MainMenuMusicComponent)
	{
		MainMenuMusicComponent->Play();
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
		if (ScoreValueText)
		{
			SGameHUD::Get().RemoveWidget(ScoreValueText);
			ScoreValueText.Reset();
		}
		if (TestImage)
		{
			SGameHUD::Get().RemoveWidget(TestImage);
			TestImage.Reset();
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
		if (ToHomeBg)
		{
			SGameHUD::Get().RemoveWidget(ToHomeBg);
			ToHomeBg.Reset();
		}
		if (ToHomeText)
		{
			SGameHUD::Get().RemoveWidget(ToHomeText);
			ToHomeText.Reset();
		}
		if (DistanceProgressBar)
		{
			SGameHUD::Get().RemoveWidget(DistanceProgressBar);
			DistanceProgressBar.Reset();
		}
		if (ObjectiveBg)
		{
			SGameHUD::Get().RemoveWidget(ObjectiveBg);
			ObjectiveBg.Reset();
		}
		if (ObjectiveImage)
		{
			SGameHUD::Get().RemoveWidget(ObjectiveImage);
			ObjectiveImage.Reset();
		}
		if (ElapsedTimeBg)
		{
			SGameHUD::Get().RemoveWidget(ElapsedTimeBg);
			ElapsedTimeBg.Reset();
		}
		if (ElapsedTimeText)
		{
			SGameHUD::Get().RemoveWidget(ElapsedTimeText);
			ElapsedTimeText.Reset();
		}
		if (BoosterProgressBar)
		{
			SGameHUD::Get().RemoveWidget(BoosterProgressBar);
			BoosterProgressBar.Reset();
		}
		if (BoosterTextBg)
		{
			SGameHUD::Get().RemoveWidget(BoosterTextBg);
			BoosterTextBg.Reset();
		}
		if (BoosterText)
		{
			SGameHUD::Get().RemoveWidget(BoosterText);
			BoosterText.Reset();
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
		// 게임 오버 UI 정리
		if (GameOverBg)
		{
			SGameHUD::Get().RemoveWidget(GameOverBg);
			GameOverBg.Reset();
		}
		if (GameOverText)
		{
			SGameHUD::Get().RemoveWidget(GameOverText);
			GameOverText.Reset();
		}
		if (RestartButton)
		{
			SGameHUD::Get().RemoveWidget(RestartButton);
			RestartButton.Reset();
		}
		if (CreditsButton)
		{
			SGameHUD::Get().RemoveWidget(CreditsButton);
			CreditsButton.Reset();
		}
		if (QuitButton)
		{
			SGameHUD::Get().RemoveWidget(QuitButton);
			QuitButton.Reset();
		}
		if (CreditBackground)
		{
			SGameHUD::Get().RemoveWidget(CreditBackground);
			CreditBackground.Reset();
		}
		if (CreditImage)
		{
			SGameHUD::Get().RemoveWidget(CreditImage);
			CreditImage.Reset();
		}
		if (KeyBindingsBackground)
		{
			SGameHUD::Get().RemoveWidget(KeyBindingsBackground);
			KeyBindingsBackground.Reset();
		}
		if (KeyBindingsImage)
		{
			SGameHUD::Get().RemoveWidget(KeyBindingsImage);
			KeyBindingsImage.Reset();
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 상태 관리
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::ShowMainMenu()
{
	CurrentGameState = EHudGameState::MainMenu;
	MainMenuTimer = 0.f;  // 타이머 리셋
	bReadyForInput = false;  // 입력 대기 상태 리셋

	// 메인 메뉴용 카메라 회전 설정 (0, 0, 0)
	APlayerController* PC = GetPlayerController();
	if (PC)
	{
		PC->SetControlRotation(FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, 0.0f)));
	}

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
	if (ScoreValueText) ScoreValueText->SetVisibility(ESlateVisibility::Hidden);
	if (TestImage) TestImage->SetVisibility(ESlateVisibility::Hidden);
	if (BoxesIcon) BoxesIcon->SetVisibility(ESlateVisibility::Hidden);
	if (BoxesLeftText) BoxesLeftText->SetVisibility(ESlateVisibility::Hidden);
	if (BoxesCountText) BoxesCountText->SetVisibility(ESlateVisibility::Hidden);
	if (CartImage) CartImage->SetVisibility(ESlateVisibility::Hidden);
	if (VehicleInfoPanel) VehicleInfoPanel->SetVisibility(ESlateVisibility::Hidden);
	if (Minimap) Minimap->SetVisibility(ESlateVisibility::Hidden);
	if (ElapsedTimeBg) ElapsedTimeBg->SetVisibility(ESlateVisibility::Hidden);
	if (ElapsedTimeText) ElapsedTimeText->SetVisibility(ESlateVisibility::Hidden);
	if (ToHomeBg) ToHomeBg->SetVisibility(ESlateVisibility::Hidden);
	if (ToHomeText) ToHomeText->SetVisibility(ESlateVisibility::Hidden);
	if (DistanceProgressBar) DistanceProgressBar->SetVisibility(ESlateVisibility::Hidden);
	if (BoosterProgressBar) BoosterProgressBar->SetVisibility(ESlateVisibility::Hidden);
	if (BoosterTextBg) BoosterTextBg->SetVisibility(ESlateVisibility::Hidden);
	if (BoosterText) BoosterText->SetVisibility(ESlateVisibility::Hidden);
	if (ObjectiveBg) ObjectiveBg->SetVisibility(ESlateVisibility::Hidden);
	if (ObjectiveImage) ObjectiveImage->SetVisibility(ESlateVisibility::Hidden);

	if (PC)
	{
		APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager();
		if (CameraManager)
		{
			CameraManager->StartFade(5.0f, 1.0f, 0.0f, FLinearColor(0.0f, 0.0f, 0.0f), 100);
		}
	}
}

void AHudExampleGameMode::StartCameraCinematic()
{
	CurrentGameState = EHudGameState::Tutorial_Camera;

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

	// 조작키 설명서는 카메라 블렌드 완료 후에 표시 (Tick에서 처리)
}

void AHudExampleGameMode::ShowTutorialComic()
{
	CurrentGameState = EHudGameState::Tutorial_Comic;

	// Press Any Key UI 숨기기 (두 번째 위젯)
	if (PressSpaceBg2)
		PressSpaceBg2->SetVisibility(ESlateVisibility::Hidden);
	if (PressSpaceImage2)
		PressSpaceImage2->SetVisibility(ESlateVisibility::Hidden);

	// 만화 배경 보이기 (검정색 전체 화면)
	if (ComicBackground)
		ComicBackground->SetVisibility(ESlateVisibility::Visible);

	// 만화 초기화
	CurrentComicIndex = 0;
	ComicSceneTimer = 0.f;
	bComicInputReady = false;  // 입력 준비 안됨 (무시 시간 후 초기화 필요)
	bComicSlideAnimating = false;
	ComicSlideTimer = 0.f;

	// 뷰포트 크기 가져오기
	FVector2D ViewportSize = SGameHUD::Get().GetViewportSize();

	// 모든 만화 이미지 초기 위치 설정
	for (int32 i = 0; i < ComicImageSlots.Num(); i++)
	{
		if (ComicImageSlots[i])
		{
			// 첫 번째 장면(i=0)은 화면 중앙(X=0)
			// 나머지는 화면 오른쪽 밖(X=ViewportSize.X)
			float InitialX = (i == 0) ? 0.f : ViewportSize.X;
			ComicImageSlots[i]->SetOffset(InitialX, 0.f);
		}
	}

	// 첫 번째 만화 장면만 표시
	for (int32 i = 0; i < ComicImages.Num(); i++)
	{
		if (ComicImages[i])
		{
			ComicImages[i]->SetVisibility((i == 0) ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void AHudExampleGameMode::ShowEndingCredits()
{
	CurrentGameState = EHudGameState::EndingCredits;

	// 게임 오버 UI 숨기기
	if (GameOverBg)
		GameOverBg->SetVisibility(ESlateVisibility::Hidden);
	if (GameOverText)
		GameOverText->SetVisibility(ESlateVisibility::Hidden);
	if (RestartButton)
		RestartButton->SetVisibility(ESlateVisibility::Hidden);
	if (CreditsButton)
		CreditsButton->SetVisibility(ESlateVisibility::Hidden);
	if (QuitButton)
		QuitButton->SetVisibility(ESlateVisibility::Hidden);

	// 엔딩 크레딧 UI 표시
	if (CreditBackground)
		CreditBackground->SetVisibility(ESlateVisibility::Visible);
	if (CreditImage)
		CreditImage->SetVisibility(ESlateVisibility::Visible);

	// 크레딧 스크롤 초기화
	CreditScrollTimer = 0.0f;
	bCreditScrollFinished = false;
	CreditEndPauseTimer = 0.0f;

	// 크레딧 이미지를 화면 좌상단에 배치하고 뷰포트 가로에 맞춰 균등 스케일
	if (CreditImageSlot && CreditImage)
	{
		// 뷰포트 위치와 크기 가져오기
		FVector2D ViewportPosition = SGameHUD::Get().GetViewportPosition();
		FVector2D ViewportSize = SGameHUD::Get().GetViewportSize();

		// 디버그: 값 확인
		UE_LOG("EndingCredits - ViewportPosition: (%.2f, %.2f), ViewportSize: (%.2f, %.2f)",
			ViewportPosition.X, ViewportPosition.Y, ViewportSize.X, ViewportSize.Y);

		// 이미지 원본 크기 가져오기
		FVector2D OriginalImageSize = CreditImage->GetImageSize();
		UE_LOG("EndingCredits - OriginalImageSize: (%.2f, %.2f)", OriginalImageSize.X, OriginalImageSize.Y);

		// 원본 종횡비 계산
		float AspectRatio = OriginalImageSize.Y / OriginalImageSize.X;

		// 뷰포트 가로에 맞춰 균등 스케일
		float ScaledWidth = ViewportSize.X;
		float ScaledHeight = ScaledWidth * AspectRatio;

		UE_LOG("EndingCredits - ScaledSize: (%.2f, %.2f)", ScaledWidth, ScaledHeight);

		// 이미지 크기 설정
		CreditImageSlot->SetSize(ScaledWidth, ScaledHeight);

		// 화면 좌상단 (0, 0)에서 시작 (뷰포트 위치가 아닌 절대 좌표)
		CreditImageSlot->SetOffset(0.f, 0.f);
		UE_LOG("EndingCredits - Starting at: (0, 0)");
	}
}

void AHudExampleGameMode::StartGamePlay()
{
	CurrentGameState = EHudGameState::Playing;

	// 메인 메뉴 UI 숨기기
	if (TitleImage)
		TitleImage->SetVisibility(ESlateVisibility::Hidden);
	if (StartButton)
		StartButton->SetVisibility(ESlateVisibility::Hidden);

	// 만화 배경 숨기기
	if (ComicBackground)
		ComicBackground->SetVisibility(ESlateVisibility::Hidden);

	// 만화 이미지들 모두 숨기기
	for (auto& Comic : ComicImages)
	{
		if (Comic)
			Comic->SetVisibility(ESlateVisibility::Hidden);
	}

	if (TestImage) TestImage->SetVisibility(ESlateVisibility::Visible);
	if (ScoreText) ScoreText->SetVisibility(ESlateVisibility::Visible);
	if (ScoreValueText) ScoreValueText->SetVisibility(ESlateVisibility::Visible);
	if (BoxesIcon) BoxesIcon->SetVisibility(ESlateVisibility::Visible);
	if (BoxesLeftText) BoxesLeftText->SetVisibility(ESlateVisibility::Visible);
	if (BoxesCountText) BoxesCountText->SetVisibility(ESlateVisibility::Visible);
	if (CartImage) CartImage->SetVisibility(ESlateVisibility::Visible);
	if (VehicleInfoPanel) VehicleInfoPanel->SetVisibility(ESlateVisibility::Visible);
	if (Minimap) Minimap->SetVisibility(ESlateVisibility::Visible);
	if (ElapsedTimeBg) ElapsedTimeBg->SetVisibility(ESlateVisibility::Visible);
	if (ElapsedTimeText) ElapsedTimeText->SetVisibility(ESlateVisibility::Visible);
	if (ToHomeBg) ToHomeBg->SetVisibility(ESlateVisibility::Visible);
	if (ToHomeText) ToHomeText->SetVisibility(ESlateVisibility::Visible);
	if (DistanceProgressBar) DistanceProgressBar->SetVisibility(ESlateVisibility::Visible);
	if (BoosterProgressBar) BoosterProgressBar->SetVisibility(ESlateVisibility::Visible);
	if (BoosterTextBg) BoosterTextBg->SetVisibility(ESlateVisibility::Visible);
	if (BoosterText) BoosterText->SetVisibility(ESlateVisibility::Visible);

	// 경과 시간 초기화
	ElapsedGameTime = 0.f;

	// 부스터 게이지 초기화
	CurrentBoosterCharge = 0.0f;

	// ─────────────────────────────────────────────────
	// Objective 연출 시작
	// ─────────────────────────────────────────────────

	ObjectiveAnimationPhase = 1;  // 등장 단계
	ObjectiveAnimationTimer = 0.0f;

	// Objective UI 표시 (투명도 0, 작은 크기로 시작)
	if (ObjectiveBg)
	{
		ObjectiveBg->SetVisibility(ESlateVisibility::Visible);
		// SGradientBox는 SetOpacity가 없으므로 Color의 alpha로 조절
		ObjectiveBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.0f));  // 완전 투명
	}
	if (ObjectiveImage)
	{
		ObjectiveImage->SetVisibility(ESlateVisibility::Visible);
		ObjectiveImage->SetOpacity(0.0f);
		ObjectiveImage->SetScale(FVector2D(0.5f, 0.5f));  // 50% 크기로 시작
	}

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

			// 컨트롤러 회전도 180도로 설정 (게임 플레이용)
			PC->SetControlRotation(FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, 180.0f)));     
	

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

	if (MainMenuMusicComponent)
	{
		MainMenuMusicComponent->Stop();
	}

	if (BackgroundMusicComponent && bIsBackgroundMusicEnabled)
	{
		BackgroundMusicComponent->Play();
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 라이프사이클
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::StartGame()
{
	Super::StartGame();
}

void AHudExampleGameMode::EndGame(bool bVictory)
{
	// 이미 게임 종료 상태면 무시
	if (bGameEndTriggered)
		return;

	bGameEndTriggered = true;

	// 부모 클래스 EndGame 호출 (GameState 설정 등)
	Super::EndGame(bVictory);

	CurrentGameState = EHudGameState::EndMenu;

	// 조작키 설명서가 열려있으면 닫기
	if (bShowKeyBindings)
	{
		bShowKeyBindings = false;
		if (KeyBindingsBackground)
			KeyBindingsBackground->SetVisibility(ESlateVisibility::Hidden);
		if (KeyBindingsImage)
			KeyBindingsImage->SetVisibility(ESlateVisibility::Hidden);
	}

	// 승리/실패에 따라 다른 UI 표시
	if (bVictory)
	{
		// 승리 UI - 경과 시간 표시
		if (GameOverText)
		{
			int32 TotalSeconds = static_cast<int32>(ElapsedGameTime);
			int32 Minutes = TotalSeconds / 60;
			int32 Seconds = TotalSeconds % 60;

			APawn* PlayerPawn = GetPlayerController()->GetPawn();
			if (!PlayerPawn) return;

			AVehicle* Vehicle = Cast<AVehicle>(PlayerPawn);
			if (!Vehicle) return;

			UCargoComponent* CargoComp = Cast<UCargoComponent>(Vehicle->GetComponent(UCargoComponent::StaticClass()));

			int32 CargoScore = CargoComp->GetValidCargoCount() * 100;
			int32 TimeScore = 2000 - TotalSeconds * 10;
			int32 Score = CargoScore + TimeScore;
			wchar_t TimeStr[128];
			swprintf_s(TimeStr, L"MISSION COMPLETE!\n\nTime: %02d:%02d\nScore : %d", Minutes, Seconds, Score);
			GameOverText->SetText(TimeStr);
		}
	}
	else
	{
		// ===== 실패 시: WASTED 애니메이션 시작 =====
		DeathPhase = EDeathAnimationPhase::WastedFadeIn;
		PhaseTimer = 0.0f;

		// WASTED 배경과 이미지 표시 준비
		if (WastedBackground)
		{
			WastedBackground->SetVisibility(ESlateVisibility::Visible);
			WastedBackground->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.0f));  // 투명 상태로 시작
		}
		if (WastedImage)
		{
			WastedImage->SetVisibility(ESlateVisibility::Visible);
			WastedImage->SetOpacity(0.0f);  // 투명 상태로 시작
		}

		// 메뉴 UI는 일단 숨김 (나중에 MenuShow 단계에서 표시)
		if (GameOverBg)
			GameOverBg->SetVisibility(ESlateVisibility::Hidden);
		if (GameOverText)
			GameOverText->SetVisibility(ESlateVisibility::Hidden);
		if (RestartButton)
			RestartButton->SetVisibility(ESlateVisibility::Hidden);
		if (CreditsButton)
			CreditsButton->SetVisibility(ESlateVisibility::Hidden);
		if (QuitButton)
			QuitButton->SetVisibility(ESlateVisibility::Hidden);

		// 버튼 텍스트는 미리 설정 (나중에 표시될 때 사용)
		if (RestartButton)
			RestartButton->SetText(L"Restart Game");
		if (CreditsButton)
			CreditsButton->SetText(L"Ending Credits");
		if (QuitButton)
			QuitButton->SetText(L"Quit Game");

		// GameOverText 텍스트도 미리 설정
		if (GameOverText)
			GameOverText->SetText(L"MISSION FAILED").SetColor(FSlateColor(1.0f, 0.0f, 0.0f, 1.0f));

		return;  // 실패 시에는 여기서 종료 (승리 시 UI만 아래에서 표시)
	}

	// ===== 승리 시에만 즉시 UI 표시 =====
	// 게임 오버 UI 표시
	if (GameOverBg)
		GameOverBg->SetVisibility(ESlateVisibility::Visible);
	if (GameOverText)
		GameOverText->SetVisibility(ESlateVisibility::Visible);

	// 버튼 텍스트를 EndMenu 버전으로 설정
	if (RestartButton)
	{
		RestartButton->SetText(L"Restart Game");
		RestartButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (CreditsButton)
	{
		CreditsButton->SetText(L"Ending Credits");
		CreditsButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (QuitButton)
	{
		QuitButton->SetText(L"Quit Game");
		QuitButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void AHudExampleGameMode::RestartGame()
{
	// 튜토리얼 스킵 플래그 설정
	bSkipTutorialOnRestart = true;

	// 델리게이트 브로드캐스트
	OnGameRestarted.Broadcast();

	// 전체 재시작 (박스/레벨 상태 포함)
	GEngine.RestartPIE();
}

// ────────────────────────────────────────────────────────────────────────────
// UI 업데이트
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::UpdateScoreUI(int32 Score)
{
	if (ScoreValueText)
	{
		ScoreValueText->SetText(std::to_wstring(Score));
	}
}

void AHudExampleGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// ─────────────────────────────────────────────────
	// Tab 키로 조작키 설명서 토글 (게임 플레이 중에만)
	// ─────────────────────────────────────────────────
	bool bTabPressed = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;

	// 게임 플레이 중에만 Tab 키로 토글
	if (CurrentGameState == EHudGameState::Playing)
	{
		// Tab 키를 새로 눌렀을 때 (이전 프레임에는 안 눌려있었고 현재 눌림)
		if (bTabPressed && !bPrevTabPressed)
		{
			// 토글
			bShowKeyBindings = !bShowKeyBindings;

			// UI 표시/숨김
			if (KeyBindingsBackground)
			{
				KeyBindingsBackground->SetVisibility(
					bShowKeyBindings ? ESlateVisibility::Visible : ESlateVisibility::Hidden
				);
			}
			if (KeyBindingsImage)
			{
				KeyBindingsImage->SetVisibility(
					bShowKeyBindings ? ESlateVisibility::Visible : ESlateVisibility::Hidden
				);
			}
		}
	}

	// 이전 프레임 키 상태 저장
	bPrevTabPressed = bTabPressed;

	// ─────────────────────────────────────────────────
	// H 키로 모든 UI 숨김 토글 (게임 플레이 중에만)
	// ─────────────────────────────────────────────────
	bool bHideUIPressed = (GetAsyncKeyState('H') & 0x8000) != 0;

	if (CurrentGameState == EHudGameState::Playing)
	{
		// H 키를 새로 눌렀을 때
		if (bHideUIPressed && !bPrevHideUIPressed)
		{
			// 토글
			bHideAllUI = !bHideAllUI;

			// 모든 게임플레이 UI 숨김/표시
			ESlateVisibility TargetVisibility = bHideAllUI ? ESlateVisibility::Hidden : ESlateVisibility::Visible;

			// 좌측 상단 UI (Reach Home)
			if (ReachHomeBg)
				ReachHomeBg->SetVisibility(TargetVisibility);
			if (ReachHomeText)
				ReachHomeText->SetVisibility(TargetVisibility);

			// Objective UI
			if (ObjectiveBg)
				ObjectiveBg->SetVisibility(TargetVisibility);
			if (ObjectiveImage)
				ObjectiveImage->SetVisibility(TargetVisibility);

			// 좌측 하단 차량 정보 패널
			if (VehicleInfoPanel)
				VehicleInfoPanel->SetVisibility(TargetVisibility);
			if (VehicleSpeedText)
				VehicleSpeedText->SetVisibility(TargetVisibility);
			if (VehicleRpmText)
				VehicleRpmText->SetVisibility(TargetVisibility);
			if (VehicleGearText)
				VehicleGearText->SetVisibility(TargetVisibility);

			// 우측 상단 타이머/점수
			if (TestImage)
				TestImage->SetVisibility(TargetVisibility);
			if (ScoreText)
				ScoreText->SetVisibility(TargetVisibility);
			if (ScoreValueText)
				ScoreValueText->SetVisibility(TargetVisibility);

			// 경과 시간 UI
			if (ElapsedTimeBg)
				ElapsedTimeBg->SetVisibility(TargetVisibility);
			if (ElapsedTimeText)
				ElapsedTimeText->SetVisibility(TargetVisibility);

			// 부스터 게이지 및 텍스트
			if (BoosterProgressBar)
				BoosterProgressBar->SetVisibility(TargetVisibility);
			if (BoosterTextBg)
				BoosterTextBg->SetVisibility(TargetVisibility);
			if (BoosterText)
				BoosterText->SetVisibility(TargetVisibility);

			// 미니맵
			if (Minimap)
				Minimap->SetVisibility(TargetVisibility);

			// 거리 진행 바
			if (ToHomeBg)
				ToHomeBg->SetVisibility(TargetVisibility);
			if (ToHomeText)
				ToHomeText->SetVisibility(TargetVisibility);
			if (DistanceProgressBar)
				DistanceProgressBar->SetVisibility(TargetVisibility);

			// 박스 개수 UI
			if (CartImage)
				CartImage->SetVisibility(TargetVisibility);
			if (BoxesIcon)
				BoxesIcon->SetVisibility(TargetVisibility);
			if (BoxesLeftBg)
				BoxesLeftBg->SetVisibility(TargetVisibility);
			if (BoxesLeftText)
				BoxesLeftText->SetVisibility(TargetVisibility);
			if (BoxesCountText)
				BoxesCountText->SetVisibility(TargetVisibility);

			// 조작키 설명서도 숨김
			if (bHideAllUI)
			{
				if (KeyBindingsBackground)
					KeyBindingsBackground->SetVisibility(ESlateVisibility::Hidden);
				if (KeyBindingsImage)
					KeyBindingsImage->SetVisibility(ESlateVisibility::Hidden);
				bShowKeyBindings = false;
			}
		}
	}

	// 이전 프레임 키 상태 저장
	bPrevHideUIPressed = bHideUIPressed;

	// ─────────────────────────────────────────────────
	// ESC 키로 일시정지 메뉴 토글 (게임 플레이 중 또는 일시정지 중)
	// ─────────────────────────────────────────────────
	bool bEscPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

	// 게임 플레이 중일 때 ESC 키를 누르면 일시정지
	if (CurrentGameState == EHudGameState::Playing)
	{
		if (bEscPressed && !bPrevEscPressed)
		{
			ShowPauseMenu();
		}
	}
	// 일시정지 중일 때 ESC 키를 누르면 게임 재개
	else if (CurrentGameState == EHudGameState::Paused)
	{
		if (bEscPressed && !bPrevEscPressed)
		{
			ResumeGame();
		}
	}

	// 이전 프레임 ESC 키 상태 저장
	bPrevEscPressed = bEscPressed;

	// 배경 음악 M키 온/오프 토글
	bool bBackgroundMusicTogglePressed = (GetAsyncKeyState('M') & 0x8000) != 0;
	if (BackgroundMusicComponent &&
		(CurrentGameState == EHudGameState::Playing ||
		 CurrentGameState == EHudGameState::EndMenu ||
		 CurrentGameState == EHudGameState::EndingCredits))
	{
		if (bBackgroundMusicTogglePressed && !bPrevBackgroundMusicTogglePressed)
		{
			bIsBackgroundMusicEnabled = !bIsBackgroundMusicEnabled;

			if (bIsBackgroundMusicEnabled)
			{
				BackgroundMusicComponent->Play();
			}
			else
			{
				BackgroundMusicComponent->Stop();
			}
		}
	}
	bPrevBackgroundMusicTogglePressed = bBackgroundMusicTogglePressed;

	// >= 쓰면 계속 FrameTime 더해줘야함
	if (FrameStableTimer < FrameStableTime && CurrentGameState == EHudGameState::Playing)
	{
		FrameStableTimer += DeltaSeconds;

		if (FrameStableTimer >= FrameStableTime)
		{
			APlayerController* PC = GetPlayerController();
			if (PC)
			{
				APawn* PlayerPawn = PC->GetPawn();
				if (PlayerPawn)
				{
					PlayerPawn->AddNewComponent(UCargoComponent::StaticClass(), PlayerPawn->RootComponent);
				}
			}
		}
	}
	// ─────────────────────────────────────────────────
	// 첫 프레임: 플레이어 카메라(스프링암)로 명시적 설정
	// ─────────────────────────────────────────────────

	if (!bInitialCameraSet && CurrentGameState == EHudGameState::MainMenu)
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

	if (CurrentGameState == EHudGameState::MainMenu)
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
			float RawAlpha = (std::sin(MainMenuTimer * 3.14159f * 2.0f / BlinkSpeed) + 1.0f) * 0.5f;  // 0~1 사이 값

			// 제곱해서 밝은 시간을 더 길게 만듦
			// 1.0에서 빼서 반전 → 제곱 → 다시 반전 (밝은 쪽으로 치우침)
			float Alpha = 1.0f - std::pow(1.0f - RawAlpha, 2.0f);

			// 최소 투명도 0.6, 최대 0.9 사이로 조정 (너무 밝지 않게)
			float FinalAlpha = 0.6f + (Alpha * 0.3f);

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
			if (UInputManager::GetInstance().IsAnyGamepadKeyPressed())
			{
				bAnyKeyPressed = true;
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

	if (CurrentGameState == EHudGameState::Tutorial_Camera)
	{
		TutorialCameraTimer += DeltaSeconds;

		// 카메라 블렌드 완료 후 조작키 설명서 표시
		if (TutorialCameraTimer >= CameraBlendTime + 0.1f && KeyBindingsImage &&
			KeyBindingsImage->GetVisibility() != ESlateVisibility::Visible)
		{
			KeyBindingsImage->SetVisibility(ESlateVisibility::Visible);
		}

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
			float RawAlpha = (std::sin(TutorialCameraTimer * 3.14159f * 2.0f / BlinkSpeed) + 1.0f) * 0.5f;

			// 제곱해서 밝은 시간을 더 길게 만듦
			// 1.0에서 빼서 반전 → 제곱 → 다시 반전 (밝은 쪽으로 치우침)
			float Alpha = 1.0f - std::pow(1.0f - RawAlpha, 2.0f);

			// 최소 투명도 0.6, 최대 0.9 사이로 조정
			float FinalAlpha = 0.6f + (Alpha * 0.3f);

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

			if (UInputManager::GetInstance().IsAnyGamepadKeyPressed())
			{
				bAnyKeyPressed = true;
			}

			if (bAnyKeyPressed)
			{
				// 조작키 설명서 숨기기
				if (KeyBindingsImage)
					KeyBindingsImage->SetVisibility(ESlateVisibility::Hidden);

				// 만화/컷씬으로 전환
				ShowTutorialComic();
				return;
			}
		}
	}

	// ─────────────────────────────────────────────────
	// 만화/컷씬 진행
	// ─────────────────────────────────────────────────

	if (CurrentGameState == EHudGameState::Tutorial_Comic)
	{
		// 슬라이드 애니메이션 처리
		if (bComicSlideAnimating)
		{
			ComicSlideTimer += DeltaSeconds;
			float Progress = FMath::Clamp(ComicSlideTimer / ComicSlideDuration, 0.0f, 1.0f);

			// Ease Out Cubic
			float EasedProgress = 1.0f - std::pow(1.0f - Progress, 3.0f);

			// 뷰포트 크기
			FVector2D ViewportSize = SGameHUD::Get().GetViewportSize();

			// 이전 장면(CurrentComicIndex-1)과 현재 장면(CurrentComicIndex)만 애니메이션
			int32 PreviousIndex = CurrentComicIndex - 1;

			// 이전 장면: 중앙(0) → 왼쪽(-ViewportSize.X)
			if (PreviousIndex >= 0 && ComicImageSlots[PreviousIndex] && ComicImages[PreviousIndex] &&
				ComicImages[PreviousIndex]->GetVisibility() == ESlateVisibility::Visible)
			{
				float StartX = 0.f;
				float EndX = -ViewportSize.X;
				float CurrentX = StartX + (EndX - StartX) * EasedProgress;
				ComicImageSlots[PreviousIndex]->SetOffset(CurrentX, 0.f);
			}

			// 현재 장면: 오른쪽(ViewportSize.X) → 중앙(0)
			if (ComicImageSlots[CurrentComicIndex] && ComicImages[CurrentComicIndex] &&
				ComicImages[CurrentComicIndex]->GetVisibility() == ESlateVisibility::Visible)
			{
				float StartX = ViewportSize.X;
				float EndX = 0.f;
				float CurrentX = StartX + (EndX - StartX) * EasedProgress;
				ComicImageSlots[CurrentComicIndex]->SetOffset(CurrentX, 0.f);
			}

			// 애니메이션 완료
			if (Progress >= 1.0f)
			{
				bComicSlideAnimating = false;
				ComicSceneTimer = 0.f;  // 타이머 리셋
				bComicInputReady = false;  // 입력 준비 상태도 리셋 (다음 장면을 위해)

				// 이전 장면을 숨김 (더 이상 화면에 보이지 않으므로)
				if (PreviousIndex >= 0 && ComicImages[PreviousIndex])
				{
					ComicImages[PreviousIndex]->SetVisibility(ESlateVisibility::Hidden);
				}
			}

			return;  // 애니메이션 중에는 입력 무시
		}

		ComicSceneTimer += DeltaSeconds;

		// 입력 체크 (ESC: 전체 스킵만 가능, 자동 진행만 허용)
		bool bNextScene = false;
		bool bSkipAll = false;

		// ESC 키로 전체 스킵 (게임패드 START 키도 스킵)
		if (UInputManager::GetInstance().IsKeyPressed(VK_ESCAPE) ||
			UInputManager::GetInstance().IsKeyPressed((int32)EGamepadButton::START))
		{
			bSkipAll = true;
		}

		// 자동 진행 (3초 경과)
		if (!bSkipAll && ComicSceneTimer >= ComicSceneWaitTime)
		{
			bNextScene = true;
		}

		// 전체 스킵
		if (bSkipAll)
		{
			// 만화 배경 숨김
			if (ComicBackground)
				ComicBackground->SetVisibility(ESlateVisibility::Hidden);

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
			// 다음 장면 인덱스
			CurrentComicIndex++;

			// 마지막 장면이었으면 게임 플레이로
			if (CurrentComicIndex >= TotalComicScenes)
			{
				// 만화 배경 숨김
				if (ComicBackground)
					ComicBackground->SetVisibility(ESlateVisibility::Hidden);

				// 모든 만화 장면 숨김
				for (auto& Comic : ComicImages)
				{
					if (Comic)
						Comic->SetVisibility(ESlateVisibility::Hidden);
				}

				StartGamePlay();
				return;
			}

			// 뷰포트 크기
			FVector2D ViewportSize = SGameHUD::Get().GetViewportSize();

			// 이전 장면(현재 화면 중앙에 있는 장면)을 다시 보이도록 설정
			// 애니메이션 중에 함께 슬라이드되어야 함
			int32 PreviousIndex = CurrentComicIndex - 1;
			if (PreviousIndex >= 0 && PreviousIndex < ComicImages.Num())
			{
				if (ComicImages[PreviousIndex])
				{
					// 이전 장면을 다시 보이게 함 (애니메이션 끝나고 숨겨졌을 수 있으므로)
					ComicImages[PreviousIndex]->SetVisibility(ESlateVisibility::Visible);
				}

				// 이전 장면의 시작 위치를 명시적으로 중앙(0)으로 설정
				if (ComicImageSlots[PreviousIndex])
				{
					ComicImageSlots[PreviousIndex]->SetOffset(0.f, 0.f);
				}
			}

			// 다음 장면 표시 (오른쪽 밖에서 시작)
			if (CurrentComicIndex < ComicImages.Num())
			{
				if (ComicImages[CurrentComicIndex])
				{
					ComicImages[CurrentComicIndex]->SetVisibility(ESlateVisibility::Visible);
				}

				// 다음 장면을 오른쪽 밖에 배치
				if (ComicImageSlots[CurrentComicIndex])
				{
					ComicImageSlots[CurrentComicIndex]->SetOffset(ViewportSize.X, 0.f);
				}
			}

			// 슬라이드 애니메이션 시작
			bComicSlideAnimating = true;
			ComicSlideTimer = 0.f;
		}
	}

	if (CurrentGameState == EHudGameState::EndMenu &&
		RestartButton.IsValid() &&
		CreditsButton.IsValid() &&
		QuitButton.IsValid())
	{
		SButton* EndMenuButtons[3] = { RestartButton.Get(), CreditsButton.Get(), QuitButton.Get()};
		const int NumButtons = 3;
		UInputManager& InputManager = UInputManager::GetInstance();
		SelectionDelay -= World->GetDeltaTime(EDeltaTime::Unscaled);
		if (SelectionDelay <= 0.0f)
		{
			float LStickYInput = InputManager.GetGamepadAxisValue((int)EGamepadAxis::LSTICK_Y);
			if (LStickYInput > 0.0f)
			{
				if (CurrentEndMenuButtonIndex != -1)
				{
					EndMenuButtons[CurrentEndMenuButtonIndex]->OnMouseLeave();
				}
				CurrentEndMenuButtonIndex = (CurrentEndMenuButtonIndex <= 0) ? NumButtons - 1 : CurrentEndMenuButtonIndex - 1;
				EndMenuButtons[CurrentEndMenuButtonIndex]->OnMouseEnter();

				SelectionDelay = 0.25f;
			}
			else if (LStickYInput < 0.0f)
			{
				if (CurrentEndMenuButtonIndex != -1)
				{
					EndMenuButtons[CurrentEndMenuButtonIndex]->OnMouseLeave();
				}
				CurrentEndMenuButtonIndex = (CurrentEndMenuButtonIndex >= NumButtons - 1) ? 0 : CurrentEndMenuButtonIndex + 1;
				EndMenuButtons[CurrentEndMenuButtonIndex]->OnMouseEnter();
				SelectionDelay = 0.25f;
			}
		}

		if (InputManager.IsKeyPressed((int)EGamepadButton::A))
		{
			// NumButtons이상일 수가 없지만 방어코드
			if (CurrentEndMenuButtonIndex >= 0 && CurrentEndMenuButtonIndex < NumButtons)
			{
				EndMenuButtons[CurrentEndMenuButtonIndex]->Click();
			}
		}
	}
	// ─────────────────────────────────────────────────
	// 엔딩 크레딧 스크롤 애니메이션
	// ─────────────────────────────────────────────────

	if (CurrentGameState == EHudGameState::EndingCredits)
	{
		// ESC 키로 엔딩 크레딧 종료
		if (UInputManager::GetInstance().IsKeyPressed(VK_ESCAPE))
		{
			// 엔딩 크레딧 숨기기
			if (CreditBackground)
				CreditBackground->SetVisibility(ESlateVisibility::Hidden);
			if (CreditImage)
				CreditImage->SetVisibility(ESlateVisibility::Hidden);

			// 게임 오버 UI 다시 표시
			if (GameOverBg)
				GameOverBg->SetVisibility(ESlateVisibility::Visible);
			if (GameOverText)
				GameOverText->SetVisibility(ESlateVisibility::Visible);
			if (RestartButton)
				RestartButton->SetVisibility(ESlateVisibility::Visible);
			if (CreditsButton)
				CreditsButton->SetVisibility(ESlateVisibility::Visible);
			if (QuitButton)
				QuitButton->SetVisibility(ESlateVisibility::Visible);

			// 상태 복원 (게임 종료 상태로)
			CurrentGameState = EHudGameState::Playing;  // 임시로 Playing 상태로
			return;
		}

		// 크레딧 스크롤이 끝났으면, 잠시 대기
		if (bCreditScrollFinished)
		{
			CreditEndPauseTimer += DeltaSeconds;
			if (CreditEndPauseTimer >= CreditEndPauseDuration)
			{
				// 엔딩 크레딧 숨기기
				if (CreditBackground)
					CreditBackground->SetVisibility(ESlateVisibility::Hidden);
				if (CreditImage)
					CreditImage->SetVisibility(ESlateVisibility::Hidden);

				// 게임 오버 UI 다시 표시
				if (GameOverBg)
					GameOverBg->SetVisibility(ESlateVisibility::Visible);
				if (GameOverText)
					GameOverText->SetVisibility(ESlateVisibility::Visible);
				if (RestartButton)
					RestartButton->SetVisibility(ESlateVisibility::Visible);
				if (CreditsButton)
					CreditsButton->SetVisibility(ESlateVisibility::Visible);
				if (QuitButton)
					QuitButton->SetVisibility(ESlateVisibility::Visible);

				// 상태 복원
				CurrentGameState = EHudGameState::Playing;  // 임시로 Playing 상태로
			}
			return; // 대기 중에는 아래 스크롤 로직 무시
		}


		// 스크롤 타이머 업데이트
		CreditScrollTimer += DeltaSeconds;

		// 스크롤 애니메이션 (화면 좌상단에서 시작해서 위로 이동)
		if (CreditImageSlot && CreditImage)
		{
			// 뷰포트 크기 (실시간 재계산)
			FVector2D ViewportSize = SGameHUD::Get().GetViewportSize();

			// 이미지 원본 크기 가져오기
			FVector2D OriginalImageSize = CreditImage->GetImageSize();

			// 원본 종횡비 계산
			float AspectRatio = OriginalImageSize.Y / OriginalImageSize.X;

			// 뷰포트 가로에 맞춰 균등 스케일 (실시간 재계산)
			float ScaledWidth = ViewportSize.X;
			float ScaledHeight = ScaledWidth * AspectRatio;

			// 이미지 크기 설정 (실시간 업데이트)
			CreditImageSlot->SetSize(ScaledWidth, ScaledHeight);

			// 전체 이동 거리 = 이미지 높이 - 뷰포트 높이
			// (이미지 상단이 화면 상단(0,0)에서 시작 -> 이미지 하단이 화면 하단에 닿으면 완료)
			float TotalDistance = ScaledHeight - ViewportSize.Y;

			// 이미지가 화면보다 작으면 스크롤 불필요
			if (TotalDistance < 0.0f)
				TotalDistance = 0.0f;

			// 진행률 (0 ~ 1)
			float Progress = FMath::Clamp(CreditScrollTimer / CreditScrollDuration, 0.0f, 1.0f);

			// 현재 Y 오프셋 (Anchor가 0.0, 0.0이므로 화면 좌상단(0,0)에서 시작하여 위로 스크롤)
			// 시작: Y = 0 (이미지 상단이 화면 상단)
			// 끝: Y = -TotalDistance (이미지 하단이 화면 하단에 정확히 도달)
			float CurrentOffsetY = -(TotalDistance * Progress);

			CreditImageSlot->SetOffset(0.f, CurrentOffsetY);

			// 스크롤 완료 시
			if (Progress >= 1.0f)
			{
				bCreditScrollFinished = true;
			}
		}

		return;  // 엔딩 크레딧 중에는 다른 업데이트 무시
	}

	// ─────────────────────────────────────────────────
	// Objective 연출 애니메이션 (게임 플레이 중에만)
	// ─────────────────────────────────────────────────

	if (CurrentGameState == EHudGameState::Playing && ObjectiveAnimationPhase > 0)
	{
		ObjectiveAnimationTimer += DeltaSeconds;

		// Phase 1: 등장 애니메이션 (Fade In + Scale Up)
		if (ObjectiveAnimationPhase == 1)
		{
			float Progress = FMath::Clamp(ObjectiveAnimationTimer / ObjectiveFadeInDuration, 0.0f, 1.0f);

			// Ease Out Cubic
			float EasedProgress = 1.0f - std::pow(1.0f - Progress, 3.0f);

			// 투명도: 0 → 1 (알파 값)
			float Alpha = EasedProgress;

			// 스케일: 0.5 → 1.0
			float Scale = 0.5f + (EasedProgress * 0.5f);

			// 배경: Color의 alpha로 투명도 조절
			if (ObjectiveBg)
				ObjectiveBg->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, 0.7f * Alpha));

			if (ObjectiveImage)
			{
				ObjectiveImage->SetOpacity(Alpha);
				ObjectiveImage->SetScale(FVector2D(Scale, Scale));
			}

			// 등장 완료
			if (Progress >= 1.0f)
			{
				ObjectiveAnimationPhase = 2;  // 대기 단계로
				ObjectiveAnimationTimer = 0.0f;
			}
		}
		// Phase 2: 대기 (읽는 시간)
		else if (ObjectiveAnimationPhase == 2)
		{
			if (ObjectiveAnimationTimer >= ObjectiveHoldDuration)
			{
				ObjectiveAnimationPhase = 3;  // 상단 이동 단계로
				ObjectiveAnimationTimer = 0.0f;
			}
		}
		// Phase 3: 상단으로 이동 (Reach Home 스타일과 일치)
		else if (ObjectiveAnimationPhase == 3)
		{
			float Progress = FMath::Clamp(ObjectiveAnimationTimer / ObjectiveMoveUpDuration, 0.0f, 1.0f);

			// Ease In Out Cubic
			float EasedProgress = Progress < 0.5f
				? 4.0f * Progress * Progress * Progress
				: 1.0f - std::pow(-2.0f * Progress + 2.0f, 3.0f) / 2.0f;

			// 시작 위치: 화면 중앙 (Y = 0)
			// 최종 위치: 상단 (Y Offset: -505, ReachHome 위치 근사치)
			float StartY = 0.0f;
			float EndY = -505.0f;
			float CurrentY = StartY + (EndY - StartY) * EasedProgress;

			// ObjectiveBg 크기: 400x60 -> 400x40 (ReachHomeBg와 일치)
			float BgSizeX = 400.f;
			float BgSizeY = 60.f - (20.f * EasedProgress);

			// ObjectiveImage 크기: 600x40 -> 160x30 (ReachHomeText와 일치)
			float ImageSizeX = 600.f - (440.f * EasedProgress);
			float ImageSizeY = 40.f - (10.f * EasedProgress);

			// 배경 위젯 업데이트 (슬롯 포인터 사용)
			if (ObjectiveBgSlot)
			{
				ObjectiveBgSlot->Offset.Y = CurrentY;
				ObjectiveBgSlot->Size = FVector2D(BgSizeX, BgSizeY);
			}

			// 이미지 위젯 업데이트 (슬롯 포인터 사용)
			if (ObjectiveImageSlot)
			{
				ObjectiveImageSlot->Offset.Y = CurrentY;
				ObjectiveImageSlot->Size = FVector2D(ImageSizeX, ImageSizeY);
			}

			// 이동 완료
			if (Progress >= 1.0f)
			{
				ObjectiveAnimationPhase = 0;  // 연출 종료
			}
		}
	}

	// ─────────────────────────────────────────────────
	// WASTED 애니메이션 상태 머신 (게임 상태와 무관하게 실행)
	// ─────────────────────────────────────────────────

	if (DeathPhase != EDeathAnimationPhase::None)
	{
		PhaseTimer += 0.016f;  // 고정 델타타임 (16ms, ~60fps)

		switch (DeathPhase)
		{
		case EDeathAnimationPhase::WastedFadeIn:
			// 0.5초 동안 페이드인 (배경 + 이미지)
			{
				float Alpha = FMath::Clamp(PhaseTimer / 0.5f, 0.0f, 1.0f);

				// 배경 그라데이션 페이드인 (검은색, 중앙이 진하고 좌우가 투명)
				if (WastedBackground)
					WastedBackground->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, Alpha * 0.8f));

				// WASTED 이미지 페이드인
				if (WastedImage)
					WastedImage->SetOpacity(Alpha);

				if (PhaseTimer >= 0.5f)
				{
					DeathPhase = EDeathAnimationPhase::WastedHold;
					PhaseTimer = 0.0f;
				}
			}
			break;

		case EDeathAnimationPhase::WastedHold:
			// 2.5초 동안 유지 (총 ~3초)
			if (PhaseTimer >= 2.5f)
			{
				DeathPhase = EDeathAnimationPhase::WastedFadeOut;
				PhaseTimer = 0.0f;
			}
			break;

		case EDeathAnimationPhase::WastedFadeOut:
			// 0.5초 동안 페이드아웃 (배경 + 이미지)
			{
				float Alpha = 1.0f - FMath::Clamp(PhaseTimer / 0.5f, 0.0f, 1.0f);

				// 배경 그라데이션 페이드아웃
				if (WastedBackground)
					WastedBackground->SetColor(FSlateColor(0.0f, 0.0f, 0.0f, Alpha * 0.8f));

				// WASTED 이미지 페이드아웃
				if (WastedImage)
					WastedImage->SetOpacity(Alpha);

				if (PhaseTimer >= 0.5f)
				{
					if (WastedBackground)
						WastedBackground->SetVisibility(ESlateVisibility::Hidden);
					if (WastedImage)
						WastedImage->SetVisibility(ESlateVisibility::Hidden);

					DeathPhase = EDeathAnimationPhase::MenuShow;
					PhaseTimer = 0.0f;
				}
			}
			break;

		case EDeathAnimationPhase::MenuShow:
			// 메뉴 표시 (MISSION FAILED + 버튼들)
			if (GameOverBg)
				GameOverBg->SetVisibility(ESlateVisibility::Visible);
			if (GameOverText)
				GameOverText->SetVisibility(ESlateVisibility::Visible);
			if (RestartButton)
				RestartButton->SetVisibility(ESlateVisibility::Visible);
			if (CreditsButton)
				CreditsButton->SetVisibility(ESlateVisibility::Visible);
			if (QuitButton)
				QuitButton->SetVisibility(ESlateVisibility::Visible);

			DeathPhase = EDeathAnimationPhase::None;  // 완료
			break;
		}
	}

	// ─────────────────────────────────────────────────
	// 게임 플레이 중일 때만 업데이트
	// ─────────────────────────────────────────────────

	if (CurrentGameState != EHudGameState::Playing)
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
			BoxesCountText->SetText(L"x " + std::to_wstring(BoxCount));
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
	// 부스터 게이지 업데이트
	// ─────────────────────────────────────────────────

	if (BoosterProgressBar)
	{
		// Shift 키를 누르고 있으면 게이지 충전
		if (Vehicle->IsBoosting())
		{
			CurrentBoosterCharge += DeltaSeconds * BoosterChargeRate;
			CurrentBoosterCharge = FMath::Clamp(CurrentBoosterCharge, 0.0f, 1.0f);
		}
		// Shift 키를 떼면 게이지 초기화
		else
		{
			CurrentBoosterCharge = 0.0f;
		}

		// 진행 바 업데이트
		BoosterProgressBar->SetPercent(CurrentBoosterCharge);
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
	// 경과 시간 업데이트 (큰 폰트)
	ElapsedGameTime += DeltaSeconds;
	if (ScoreValueText)
	{
		int32 TotalSeconds = static_cast<int32>(ElapsedGameTime);
		int32 Minutes = TotalSeconds / 60;
		int32 Seconds = TotalSeconds % 60;

		wchar_t TimeStr[32];
		swprintf_s(TimeStr, L"%02d:%02d", Minutes, Seconds);
		ScoreValueText->SetText(TimeStr);
	}

	// ─────────────────────────────────────────────────
	// 목적지까지 거리 진행 바 업데이트
	// ─────────────────────────────────────────────────

	if (DistanceProgressBar)
	{
		FVector StartPos(166.79f, 1.3f, 3.0f);   // 시작 위치
		FVector EndPos(53.818f, -270.202f, 3.0f); // 엔딩 위치
		FVector CurrentPos = Vehicle->GetActorLocation();

		// 시작→엔딩 총 거리 (2D, Z축 무시)
		FVector StartToEnd = EndPos - StartPos;
		StartToEnd.Z = 0.0f;  // Z축 무시
		float TotalDistance = StartToEnd.Size();

		// 현재→엔딩 남은 거리 (2D, Z축 무시)
		FVector CurrentToEnd = EndPos - CurrentPos;
		CurrentToEnd.Z = 0.0f;  // Z축 무시
		float RemainingDistance = CurrentToEnd.Size();

		// 진행률 계산 (0.0 ~ 1.0)
		float Progress = 0.0f;
		if (TotalDistance > 0.0f)
		{
			Progress = 1.0f - (RemainingDistance / TotalDistance);
			Progress = FMath::Clamp(Progress, 0.0f, 1.0f);  // 0~1 범위로 제한
		}

		DistanceProgressBar->SetPercent(Progress);
	}

	// 박스 개수 UI 업데이트
	if (BoxesCountText)
	{
		UCargoComponent* CargoComp = Cast<UCargoComponent>(Vehicle->GetComponent(UCargoComponent::StaticClass()));
		if (CargoComp)
		{
			int32 BoxCount = CargoComp->GetValidCargoCount();
			BoxesCountText->SetText(L"x " + std::to_wstring(BoxCount));
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 일시정지 메뉴 표시
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::ShowPauseMenu()
{
	// 상태를 일시정지로 변경
	CurrentGameState = EHudGameState::Paused;

	// 게임을 일시정지 (업데이트 멈춤)
	UWorld* World = GetWorld();
	if (World)
	{
		World->SetPaused(true);
	}

	// 조작키 설명서가 열려있으면 닫기
	if (bShowKeyBindings)
	{
		bShowKeyBindings = false;
		if (KeyBindingsBackground)
			KeyBindingsBackground->SetVisibility(ESlateVisibility::Hidden);
		if (KeyBindingsImage)
			KeyBindingsImage->SetVisibility(ESlateVisibility::Hidden);
	}

	// 일시정지 메뉴 UI 표시 (기존 EndMenu UI 재활용)
	if (GameOverBg)
		GameOverBg->SetVisibility(ESlateVisibility::Visible);

	if (GameOverText)
	{
		GameOverText->SetText(L"PAUSED");
		GameOverText->SetVisibility(ESlateVisibility::Visible);
	}

	// 재시작 버튼
	if (RestartButton)
	{
		RestartButton->SetText(L"Restart Game");
		RestartButton->SetVisibility(ESlateVisibility::Visible);
	}

	// 엔딩 크레딧 버튼
	if (CreditsButton)
	{
		CreditsButton->SetText(L"Resume");
		CreditsButton->SetVisibility(ESlateVisibility::Visible);
	}

	// 게임 종료 버튼
	if (QuitButton)
	{
		QuitButton->SetText(L"Quit Game");
		QuitButton->SetVisibility(ESlateVisibility::Visible);
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 게임 재개
// ────────────────────────────────────────────────────────────────────────────

void AHudExampleGameMode::ResumeGame()
{
	// 상태를 Playing으로 복원
	CurrentGameState = EHudGameState::Playing;

	// 게임 재개 (업데이트 재시작)
	UWorld* World = GetWorld();
	if (World)
	{
		World->SetPaused(false);
	}

	// 일시정지 메뉴 UI 숨기기
	if (GameOverBg)
		GameOverBg->SetVisibility(ESlateVisibility::Hidden);

	if (GameOverText)
		GameOverText->SetVisibility(ESlateVisibility::Hidden);

	if (RestartButton)
		RestartButton->SetVisibility(ESlateVisibility::Hidden);

	if (CreditsButton)
		CreditsButton->SetVisibility(ESlateVisibility::Hidden);

	if (QuitButton)
		QuitButton->SetVisibility(ESlateVisibility::Hidden);
}
